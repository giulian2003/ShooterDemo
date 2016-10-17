//
// Copyright (c) 2016 Iulian Marinescu Ghetau giulian2003@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely.  If you use this software in a product, an acknowledgment in the 
// product documentation would be appreciated but is not required.
//

#include "resources.hpp"
#include "shader_defines.h"
#include "shader_utils.hpp"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem> // requires at least VS2013

#include <SDL_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp> 
#include <glm/gtx/compatibility.hpp>

#include <assimp/Importer.hpp>
#include <assimp/PostProcess.h>
#include <assimp/Scene.h>
#include <assimp/matrix4x4.h>

using namespace std;
using namespace glm;
using namespace std::tr2::sys;
using namespace ShaderUtils;

namespace shooter {

  namespace {

    /// Changes the path 
    string GetFixedTexturePath(const char* oldFilePath, const string& altTexFolder)
    {
      path newPath = path(altTexFolder);
      path texName = path(oldFilePath).filename();
      newPath /= texName;
      return newPath.string();
    }

    void AddBoneData(uint32_t boneIndex, const aiVertexWeight& vertInfo, VertexBoneData& outVertBoneData)
    {
      for (unsigned i = 0; i < 4; i++)
      {
        if (outVertBoneData.weights[i] < FLT_EPSILON)
        {
          outVertBoneData.boneIds[i] = boneIndex;
          // BugFix: On some GPUs, the OpenGL driver always normalizes the weights even though 
          // we call glVertexAttribPointer with the normalized parameter set to false.
          // To fix this, always enable the normalization and multiply the weights by 1000, 
          // to avoid precission loss.
          outVertBoneData.weights[i] = vertInfo.mWeight * 1000.f;
          return;
        }
      }

      assert(0);
    }

    template <class taType, taType(*fnInterp)(const taType&, const taType&, float)>
    taType InterpolateKey(const vector<pair<float, taType> >& keys, float animTime, taType lastVal, float lastAnimTime)
    {
      assert(!keys.empty());

      if (animTime < -FLT_EPSILON)
      {
        // Transition from the last animation to the current animation.
        assert((lastAnimTime < -FLT_EPSILON) && (animTime >= lastAnimTime));
        float factor = 1.f - animTime / lastAnimTime;
        assert(factor > -FLT_EPSILON && factor < 1.f + FLT_EPSILON);

        return fnInterp(lastVal, keys.front().second, factor);
      }

      if (keys.size() == 1)
      {
        return keys.front().second;
      }

      auto it = upper_bound(keys.begin(), keys.end(), animTime,
        [](float val1, const pair<float, taType>& val2) { return val1 < val2.first; });

      if (it == keys.begin())
      {
        return keys.front().second;
      }

      if (it == keys.end())
      {
        return keys.back().second;
      }

      auto itPrev = it - 1;
      float factor = (animTime - itPrev->first) / (it->first - itPrev->first);
      assert(factor > -FLT_EPSILON && factor < 1.f + FLT_EPSILON);

      return fnInterp(itPrev->second, it->second, factor);
    }

  }

  Resources::~Resources() 
  {
    glDeleteBuffers(mBufferObjects.size(), mBufferObjects.data());
    for (auto pair : mPrograms) { glDeleteProgram(pair.second); }
    for (auto pair : mModels) 
    {
      glDeleteTextures(pair.second.textures.size(), pair.second.textures.data());
      for (auto mesh : pair.second.meshes) { glDeleteVertexArrays(1, &mesh.vao); }
      for (auto mat : pair.second.materialsTex)
      {
        for (auto typeAndObj : mat) { glDeleteTextures(1, &typeAndObj.second); }
      }
    }
    glDeleteTextures(1, &mSkyBoxTexture);
  }

  bool Resources::LoadMap(const std::string& zipFilePath) 
  {
    mMap.reset(new Q3Map(mResourceFolder + zipFilePath));

    if (!mMap->GetMapQ3().mVertices.size())
    {
      return false;
    }

    std::vector<int> indices;
    std::vector<float> vertices, normals;
    mMap->GetVerticesAndIndices(vertices, normals, indices);
    assert(indices.size() / 3 * 3 == indices.size());

    mNavMesh.reset(
      new NavMesh(
        1.8f, .8f,
        .5f, 60.f,
        .25f, .05f,
        10.f, .8f,
        8.f, 20.f,
        8.f, 0.9f,
        vertices.data(), normals.data(),
        indices.data(), indices.size() / 3,
        (const float *)&mMap->GetMapQ3().mNodes[0].mMins,
        (const float *)mMap->GetMapQ3().mNodes[0].mMaxs,
        6.f, 10.f, 3.f, 4.f, .9f, 18.f));

    return true;
  }

  bool Resources::LoadSkyBox(const std::string& prefix)
  {
    std::vector<std::string> extensions = { ".png", ".jpg", ".png", ".tga" };
    for (const auto& ext : extensions)
    {
      mSkyBoxTexture = ShaderUtils::LoadCubeMapTexture(
        mResourceFolder + prefix + "_rt" + ext,
        mResourceFolder + prefix + "_lf" + ext,
        mResourceFolder + prefix + "_up" + ext,
        mResourceFolder + prefix + "_dn" + ext,
        mResourceFolder + prefix + "_bk" + ext,
        mResourceFolder + prefix + "_ft" + ext);

      if (mSkyBoxTexture) { break; }
    }

    if (mSkyBoxTexture) { return true; }
    
    return false;
  }

  bool Resources::LoadPrograms(const string& folderPath)
  {
    path root(mResourceFolder + folderPath);

    // Check for correct path
    if (!exists(root) || !is_directory(root))
    {
      cout << "Invalid resource path: " << folderPath << endl;
      return false;
    }

    // Group all files with the same name in the resourcePath folder
    unordered_map<string, vector<string> > pathMap;
    for (directory_iterator it(root), itEnd; it != itEnd; ++it)
    {
      const path& p(*it);
      if (is_regular_file(p))
      {
        pathMap[p.stem().string()].push_back(p.string());
      }
    }

    string definesPath;
    auto it = pathMap.find("shader_defines");
    if ((it != end(pathMap)) && !it->second.empty())
    {
      definesPath = it->second[0];
    }

    // Load programs
    for (const auto& pair : pathMap)
    {
      uint32_t programId = LoadProgram(pair.second, definesPath);
      if (programId > 0)
      {
        mPrograms[pair.first] = programId;
      }
    }

    if (mPrograms.empty()) { return false; }

    return true;
  }

  bool Resources::LoadModel(const std::string& filePath)
  {
    Assimp::Importer importer;

    // The Assimp scene object
    const aiScene* scene = importer.ReadFile(mResourceFolder + filePath, aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs);
    if (!scene)
    {
      cout << importer.GetErrorString() << endl;
      return false;
    }

    Model& model = mModels[filePath];

    model.globalInvTrans = glm::inverse(glm::transpose(make_mat4(&scene->mRootNode->mTransformation.a1)));
    ProcessNodeHierarchy(scene, scene->mRootNode, model, -1);
    LoadEmbeddedTextures(scene, model);
    LoadMaterials(scene, mResourceFolder + path(filePath).parent_path().string(), model);
    LoadMeshes(scene, path(filePath).parent_path().string(), model, mBufferObjects);

    // We're done. Everything will be cleaned up by the importer destructor
    return true;
  }

  GLuint Resources::GetProgram(const std::string& programName) const {
    auto it = mPrograms.find(programName);
    if (it != mPrograms.end())
    {
      return it->second;
    }

    return 0;
  }

  const Model& Resources::GetModel(const std::string& modelName) const {
    auto it = mModels.find(modelName);
    if (it != mModels.end())
    {
      return it->second;
    }

    return mEmptyModel;
  }

  const Animation& Resources::GetAnimation(const Model& model, const std::string& animationName) const
  {
    auto it = model.animationsMap.find(animationName);
    if (it != model.animationsMap.end())
    {
      return it->second;
    }

    return mEmptyAnimation;
  }

  const Animation& Resources::GetAnimation(const std::string& modelName, const std::string& animationName) const
  {
    const Model& model = GetModel(modelName);
    return GetAnimation(model, animationName);
  }

  void Resources::GetSkeletonTransforms(
    const Model& model,
    const std::string& animationName,
    float animationTimeInSeconds,
    float& lastAnimationTimeInSeconds,
    std::vector<AnimationFrame>& inoutLastAnimationFrames,
    std::vector<glm::mat4>& outGlobalTransforms) const
  {
    const auto itAnim = model.animationsMap.find(animationName);
    if (itAnim == model.animationsMap.end()) 
    {
      return;
    }

    const int32_t nrNodes = model.nodesParents.size();

    inoutLastAnimationFrames.resize(nrNodes);
    outGlobalTransforms.resize(nrNodes);

    const Animation& anim(itAnim->second);

    float ticksPerSecond = anim.ticksPerSecond != 0 ? anim.ticksPerSecond : 25.0f;
    ticksPerSecond *= 2.f; // tune the animation speed
    float lastTimeInTicks = lastAnimationTimeInSeconds * ticksPerSecond;
    lastAnimationTimeInSeconds = animationTimeInSeconds;

    for (int32_t nodeIndex = 0; nodeIndex < nrNodes; nodeIndex++)
    {
      int32_t parentIndex = model.nodesParents[nodeIndex];
      assert(parentIndex < nodeIndex);

      const NodeAnimation& nodeAnim = anim.nodesAnimation[nodeIndex];
      AnimationFrame& lastFrame = inoutLastAnimationFrames[nodeIndex];

      float timeInTicks = animationTimeInSeconds * ticksPerSecond;
      if ((timeInTicks > 0.f) && (nodeAnim.postState == EAnimBehaviourRepeat))
      {
        timeInTicks = fmod(animationTimeInSeconds * ticksPerSecond, anim.durationInTicks);
      }

      mat4 nodeTransform = model.nodesTrans[nodeIndex];
      
      // calculate the skeleton transformations in local space
      if (!nodeAnim.scalings.empty() || !nodeAnim.rotations.empty() || !nodeAnim.translations.empty())
      {
        lastFrame.scaling = InterpolateKey<vec3, &lerp>(nodeAnim.scalings, timeInTicks, lastFrame.scaling, lastTimeInTicks);
        lastFrame.rotation = InterpolateKey<quat, &slerp>(nodeAnim.rotations, timeInTicks, lastFrame.rotation, lastTimeInTicks);
        lastFrame.translation = InterpolateKey<vec3, &lerp>(nodeAnim.translations, timeInTicks, lastFrame.translation, lastTimeInTicks);

        // Replace the local transformation. The order of multiplication it's important
        nodeTransform = translate(lastFrame.translation) * mat4_cast(lastFrame.rotation) * scale(lastFrame.scaling);
      }
      else
      {
        lastFrame = AnimationFrame();
      }
      
      if (nodeIndex > 0)
      {
        outGlobalTransforms[nodeIndex] = outGlobalTransforms[parentIndex] * nodeTransform;
      }
      else
      {
        outGlobalTransforms[nodeIndex] = nodeTransform;
      }
    }

    // calculate the skeleton transformations in world space
    for (int32_t nodeIndex = 0; nodeIndex < nrNodes; nodeIndex++)
    {
      outGlobalTransforms[nodeIndex] = model.globalInvTrans * outGlobalTransforms[nodeIndex] * model.bonesOffsets[nodeIndex];
    }

    return;
  }

  const aiNodeAnim* Resources::FindNodeAnim(const aiAnimation* pAnimation, const string& NodeName)
  {
    for (uint i = 0; i < pAnimation->mNumChannels; ++i)
    {
      const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

      if (NodeName == pNodeAnim->mNodeName.C_Str())
        return pNodeAnim;
    }

    return NULL;
  }

  void Resources::ProcessNodeAnim(const string& nodeName, const aiAnimation* anim, Animation& animation)
  {
    animation.durationInTicks = (float)anim->mDuration;
    animation.ticksPerSecond = (float)anim->mTicksPerSecond;

    // copy the animation frames
    animation.nodesAnimation.push_back(NodeAnimation());
    NodeAnimation& nodeAnim = animation.nodesAnimation.back();

    const aiNodeAnim* pNodeAnim = FindNodeAnim(anim, nodeName);
    if (pNodeAnim != NULL)
    {
      nodeAnim.preState = static_cast<AnimBehaviour>(pNodeAnim->mPreState);
      nodeAnim.postState = static_cast<AnimBehaviour>(pNodeAnim->mPostState);

      // HACK: assimp does not load the correct states
      if (strcmp(anim->mName.C_Str(), "Jump") == 0)
      {
        nodeAnim.postState = EAnimBehaviourConstant;
      }
      else
      {
        nodeAnim.postState = EAnimBehaviourRepeat;
      }

      nodeAnim.translations.reserve(pNodeAnim->mNumPositionKeys);
      for_each(pNodeAnim->mPositionKeys, pNodeAnim->mPositionKeys + pNodeAnim->mNumPositionKeys, [&](const aiVectorKey& key) {
        nodeAnim.translations.push_back(make_pair(float(key.mTime), vec3(key.mValue.x, key.mValue.y, key.mValue.z)));
      });

      nodeAnim.rotations.reserve(pNodeAnim->mNumRotationKeys);
      for_each(pNodeAnim->mRotationKeys, pNodeAnim->mRotationKeys + pNodeAnim->mNumRotationKeys, [&](const aiQuatKey& key) {
        nodeAnim.rotations.push_back(make_pair(float(key.mTime), quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)));
      });

      nodeAnim.scalings.reserve(pNodeAnim->mNumScalingKeys);
      for_each(pNodeAnim->mScalingKeys, pNodeAnim->mScalingKeys + pNodeAnim->mNumScalingKeys, [&](const aiVectorKey& key) {
        nodeAnim.scalings.push_back(make_pair(float(key.mTime), vec3(key.mValue.x, key.mValue.y, key.mValue.z)));
      });
    }
  }

  void Resources::ProcessNode(const aiScene* scene, const aiNode* pNode, Model& model, int32_t parentNodeIndex, int32_t nodeIndex)
  {
    string nodeName(pNode->mName.C_Str());

    model.nodesMap[nodeName] = nodeIndex;

    model.nodesParents.push_back(parentNodeIndex);

    assert(model.nodesTrans.size() == nodeIndex);
    mat4 locatTransformation = transpose(make_mat4(&pNode->mTransformation.a1));
    model.nodesTrans.push_back(locatTransformation);

    if (scene->HasAnimations())
    {
      for (uint i = 0; i < scene->mNumAnimations; i++)
      {
        const aiAnimation* anim = scene->mAnimations[i];
        Animation& animation = model.animationsMap[anim->mName.C_Str()];

        assert(animation.nodesAnimation.size() == nodeIndex);
        ProcessNodeAnim(nodeName, anim, animation);
      }
    }
  }

  void Resources::ProcessNodeHierarchy(const aiScene* scene, const aiNode* pNode, Model& model, int32_t parentNodeIndex)
  {
    uint32 nodeIndex = model.nodesParents.size();

    ProcessNode(scene, pNode, model, parentNodeIndex, nodeIndex);

    for (uint i = 0; i < pNode->mNumChildren; i++)
    {
      ProcessNodeHierarchy(scene, pNode->mChildren[i], model, nodeIndex);
    }
  }

  GLuint Resources::LoadTexture(string name, TextureMap& textureMap)
  {
    auto it = textureMap.find(name);
    if (it != textureMap.end())
    {
      return it->second;
    }
    uint32_t bpp;
    return ShaderUtils::LoadTexture(name, bpp);
  }

  void Resources::LoadEmbeddedTextures(const aiScene* scene, Model& model)
  {
    for_each(scene->mTextures, scene->mTextures + scene->mNumTextures, [&](const aiTexture* tex) {
      model.textures.push_back(ShaderUtils::LoadTexture(tex));
    });
  }

  void Resources::LoadMaterials(const aiScene* scene, const string& altPath, Model& model)
  {
    for (uint32_t matIndex = 0; matIndex < scene->mNumMaterials; matIndex++)
    {
      const aiMaterial* mtl = scene->mMaterials[matIndex];

      // process the material's textures
      model.materialsTex.push_back(MaterialTextures());
      MaterialTextures& matTex = model.materialsTex.back();
      for (int texType = 0; texType < aiTextureType_UNKNOWN; texType++)
      {
        aiTextureType aiTexType = (aiTextureType)texType;
        int texTypeCnt = mtl->GetTextureCount((aiTextureType)texType);
        for (int texTypeIndex = 0; texTypeIndex < texTypeCnt; texTypeIndex++)
        {
          GLuint texObj = 0;
          aiString texPath;
          if (AI_SUCCESS == mtl->GetTexture(aiTexType, texTypeIndex, &texPath))
          {
            if (texPath.data[0] == '*')
            {
              // embeded texture
              GLuint iIndex = atoi(texPath.C_Str() + 1);
              if (iIndex < model.textures.size())
              {
                texObj = model.textures[iIndex];
              }
            }
            else
            {
              texObj = LoadTexture(texPath.data, model.textureMap);
              if (texObj == 0)
              {
                texObj = LoadTexture(GetFixedTexturePath(texPath.data, altPath), model.textureMap);
              }
            }
            matTex.push_back(make_pair((TextureType)texType, texObj));
          }
        }
      }

      MaterialColors mat;
      aiColor4D c;

      // process the material colors
      if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &c)) 
      {
        mat.diffuse = vec4(c.r, c.g, c.b, c.a);
      }

      if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &c)) 
      {
        mat.ambient = vec4(c.r, c.g, c.b, c.a);
      }

      if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &c)) 
      {
        mat.specular = vec4(c.r, c.g, c.b, c.a);
      }

      if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &c)) 
      {
        mat.emissive = vec4(c.r, c.g, c.b, c.a);
      }

      float shininess = 0.0;
      unsigned int max;
      aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
      mat.shininess = shininess;

      mat.texCount = model.materialsTex.back().size();

      GLuint buffer = 0;
      glGenBuffers(1, &buffer);
      glBindBuffer(GL_UNIFORM_BUFFER, buffer);
      glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialColors), (void *)(&mat), GL_STATIC_DRAW);
      glBindBuffer(GL_UNIFORM_BUFFER, 0);
      model.materialsCol.push_back(buffer);
    }
  }

  vector<VertexBoneData> Resources::LoadBones(const aiMesh* pMesh, const NamesAndIdsMap& nodesMap, std::vector<mat4>& bonesOffset, std::vector<mat4>& invBonesOffset)
  {
    vector<VertexBoneData> data(pMesh->mNumVertices);
    for (uint32_t i = 0; i < pMesh->mNumBones; i++)
    {
      const aiBone* bone = pMesh->mBones[i];
      const char *boneName = pMesh->mBones[i]->mName.C_Str(); // it's the same as the node name

      auto it = nodesMap.find(boneName);
      assert(it != nodesMap.end());

      uint32_t nodeIndex = it->second;
      assert(nodeIndex < bonesOffset.size());

      mat4 newMat = transpose(make_mat4(&bone->mOffsetMatrix.a1));
      assert(bonesOffset[nodeIndex] == mat4() || bonesOffset[nodeIndex] == newMat);

      bonesOffset[nodeIndex] = newMat;
      invBonesOffset[nodeIndex] = inverse(newMat);

      for (uint32_t i = 0; i < bone->mNumWeights; ++i)
      {
        const aiVertexWeight& vertInfo = bone->mWeights[i];
        AddBoneData(nodeIndex, vertInfo, data[vertInfo.mVertexId]);
      }
    }

    return data;
  }

  void Resources::LoadMeshes(const aiScene* scene, const string& modelPath, Model& model, std::vector<GLuint>& inoutBufferObjs)
  {
    TextureMap textureMap;

    GLuint buffer = 0;

    aiVector3D minBound = { FLT_MAX, FLT_MAX, FLT_MAX };
    aiVector3D maxBound = { FLT_MIN, FLT_MIN, FLT_MIN };

    model.bonesOffsets.assign(model.nodesParents.size(), mat4());
    model.invBonesOffsets.assign(model.nodesParents.size(), mat4());

    // For each mesh
    for (uint32_t n = 0; n < scene->mNumMeshes; ++n)
    {
      Mesh aMesh = {};

      const aiMesh* mesh = scene->mMeshes[n];

      aMesh.materialIndex = mesh->mMaterialIndex;

      // update scene's bounding box
      for_each(mesh->mVertices, mesh->mVertices + mesh->mNumVertices, [&](const aiVector3D& v) 
      {
        minBound.x = glm::min(minBound.x, v.x); maxBound.x = glm::max(maxBound.x, v.x);
        minBound.y = glm::min(minBound.y, v.y); maxBound.y = glm::max(maxBound.y, v.y);
        minBound.z = glm::min(minBound.z, v.z); maxBound.z = glm::max(maxBound.z, v.z);
      });

      // generate Vertex Array for mesh
      glGenVertexArrays(1, &(aMesh.vao));
      glBindVertexArray(aMesh.vao);

      // buffer for faces
      if (mesh->HasFaces())
      {
        std::vector<uint32_t> faceArray;
        faceArray.reserve(mesh->mNumFaces * 3);
        for (uint32_t t = 0; t < mesh->mNumFaces; ++t)
        {
          const aiFace& face = mesh->mFaces[t];
          faceArray.insert(faceArray.end(), face.mIndices, face.mIndices + face.mNumIndices);
        }
        aMesh.numFaces = scene->mMeshes[n]->mNumFaces;

        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * faceArray.size(), faceArray.data(), GL_STATIC_DRAW);
        inoutBufferObjs.push_back(buffer);
      }

      // buffer for vertex positions
      if (mesh->HasPositions())
      {
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(VERT_POSITION_LOC);
        glVertexAttribPointer(VERT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, 0, 0);
        inoutBufferObjs.push_back(buffer);
      }

      // buffer for vertex normals
      if (mesh->HasNormals())
      {
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * mesh->mNumVertices, mesh->mNormals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(VERT_NORMAL_LOC);
        glVertexAttribPointer(VERT_NORMAL_LOC, 3, GL_FLOAT, GL_FALSE, 0, 0);
        inoutBufferObjs.push_back(buffer);
      }

      // buffer for vertex texture coordinates
      if (mesh->HasTextureCoords(0))
      {
        std::vector<float> texCoords;
        texCoords.reserve(2 * mesh->mNumVertices);
        for (unsigned int k = 0; k < mesh->mNumVertices; ++k)
        {
          texCoords.push_back(mesh->mTextureCoords[0][k].x);
          texCoords.push_back(mesh->mTextureCoords[0][k].y);
        }
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * mesh->mNumVertices, texCoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(VERT_DIFFUSE_TEX_COORD_LOC);
        glVertexAttribPointer(VERT_DIFFUSE_TEX_COORD_LOC, 2, GL_FLOAT, GL_FALSE, 0, 0);
        inoutBufferObjs.push_back(buffer);
      }

      // buffer for vertex bone IDs and Weights
      if (mesh->HasBones())
      {
        vector<VertexBoneData> vertexBoneData = LoadBones(mesh, model.nodesMap, model.bonesOffsets, model.invBonesOffsets);
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexBoneData) * vertexBoneData.size(), vertexBoneData.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(VERT_BONE_IDS_LOC);
        glVertexAttribIPointer(VERT_BONE_IDS_LOC, 4, GL_UNSIGNED_INT, sizeof(VertexBoneData), 0);
        glEnableVertexAttribArray(VERT_BONE_WEIGHTS_LOC);
        glVertexAttribPointer(VERT_BONE_WEIGHTS_LOC, 4, GL_FLOAT, GL_TRUE, sizeof(VertexBoneData), (const GLvoid*)16);
        inoutBufferObjs.push_back(buffer);
      }

      // unbind buffers
      glBindVertexArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      model.meshes.push_back(aMesh);
    }

    model.minBound = make_vec3(&minBound.x);
    model.maxBound = make_vec3(&maxBound.x);

    float maxBoundsSize = glm::max(maxBound.x - minBound.x, maxBound.y - minBound.y);
    maxBoundsSize = glm::max(maxBoundsSize, maxBound.z - minBound.z);

    if (maxBoundsSize > 0.0000001f)
    {
      model.normScale = 1.0f / maxBoundsSize;
    }
    else
    {
      model.normScale = 1.0f;
      cout << "Mesh with extremely small bounding box!" << endl;
    }
  }

}