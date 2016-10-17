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

#include "Q3Map.hpp"
#include "nav_mesh.hpp"
#include "Q3SurfaceFlags.hpp"
#include "resources.hpp"
#include "shader_defines.h"
#include "shader_utils.hpp"
#include "camera_utils.hpp"

#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

#include <unzip.h>
#include <SDL_image.h>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace shooter;
using namespace ShaderUtils;
using namespace glm;
using namespace std;

namespace 
{
  /// Texture type (stored on 2 bits)
  enum TexType
  {
    ETexTypeNormal = 0,
    ETexTypeTransparent,
    ETexTypeFlame,
    ETexTypeSwirl,
  };

  /// Set the texture type into the texTypeBits bit mask
  inline void SetTexType(
    uint32_t texIx, ///< Texture index
    TexType type, ///< Texture type
    std::vector<uint64_t>& texTypeBits) ///< Textures bit mask
  {
    assert(texIx < (texTypeBits.size() << 5));
    texTypeBits[texIx >> 5] |= ((uint64_t)type << (texIx % 32 << 1));
  }

  /// Get the texture type from the texTypeBits bit mask
  inline TexType GetTexType(
    uint32_t texIx, ///< Texture index
    const std::vector<uint64_t>& texTypeBits) ///< Textures bit mask
  {
    assert(texIx < (texTypeBits.size() << 5));
    return (TexType)((texTypeBits[texIx >> 5] >> (texIx % 32 << 1)) & 3);
  }

  /// Set a bit into the bit mask
  inline void SetBit(uint32_t ix, std::vector<uint64_t>& mask)
  {
    assert(ix < (mask.size() << 6));
    mask[ix >> 6] |= (1ULL << (ix % 64));
  }

  /// Get a bit from the bit mask
  inline bool IsBitSet(uint32_t ix, const std::vector<uint64_t>& mask)
  {
    assert(ix < (mask.size() << 6));
    return (mask[ix >> 6] & (1ULL << (ix % 64))) != 0ULL;
  }

  /// Calculate a hash value for a face
  inline uint32_t GetFaceHash(const TFace& face) 
  {
    return (face.mTextureIndex+1) | ((face.mLightmapIndex+1) << 16);
  }

  /// Calculates the projection a vector on a plane
  inline vec3 proj(vec3 planeNorm, vec3 v)
  {
    return v - planeNorm * dot(v, planeNorm);
  }

  inline string toLower(const char* str)
  {
    string lowerStr(str);
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
  }

  typedef pair<unz64_file_pos, uint32_t> TFilePosAndLen;
  typedef unordered_map<string, TFilePosAndLen> TArchivedFilesInfoMap;

  /// Read files names and positions from a zip archive
  TArchivedFilesInfoMap ReadArchiveFileNames(unzFile zipFileHandle, TFilePosAndLen& bspFilePosAndLen = TFilePosAndLen())
  {
    TArchivedFilesInfoMap result;

    if (zipFileHandle == NULL)
    {
      return result;
    }

    //  At first ensure file is already open
    if (unzGoToFirstFile(zipFileHandle) == UNZ_OK)
    {
      // Loop over all files
      do
      {
        char filename[255];
        unz_file_info64 fileInfo;
        unz64_file_pos filePos;

        if ((unzGetCurrentFileInfo64(zipFileHandle, &fileInfo, filename, 255, NULL, 0, NULL, 0) == UNZ_OK) &&
            (fileInfo.uncompressed_size > 0) &&
            (unzGetFilePos64(zipFileHandle, &filePos) == UNZ_OK))
        {
          TFilePosAndLen posAndLen(filePos, (uint32_t)fileInfo.uncompressed_size);

          result[filename] = posAndLen;

          if ((fileInfo.size_filename > 3) &&
            (filename[fileInfo.size_filename - 1] == 'p') &&
            (filename[fileInfo.size_filename - 2] == 's') &&
            (filename[fileInfo.size_filename - 3] == 'b'))
          {
            bspFilePosAndLen = posAndLen;
          }
        }
      } while (unzGoToNextFile(zipFileHandle) != UNZ_END_OF_LIST_OF_FILE);
    }

    return result;
  }

  /// Read the contents of a file from a zip archive
  void* ReadFileAtPos(unzFile zipFileHandle, const TFilePosAndLen& filePosAndLen)
  {
    if ((unzGoToFilePos64(zipFileHandle, &filePosAndLen.first) == UNZ_OK) &&
      (unzOpenCurrentFile(zipFileHandle) == UNZ_OK))
    {
      void *p = malloc(filePosAndLen.second);
      if (unzReadCurrentFile(zipFileHandle, p, filePosAndLen.second) == filePosAndLen.second)
      {
        return p;
      }
      free(p);
    }

    return nullptr;
  }
}

/// Load a texture from a zip archive to the GPU memory
void LoadTexture(
  const TTexture& texture, 
  const unzFile fileHandle, 
  const TArchivedFilesInfoMap& filesMap,
  std::vector<GLuint>& textures,
  std::vector<uint64_t>& texturesTypeBits)
{
  static const vector<string> cExtensions = { ".jpg", ".tga", ".png" };
  GLuint tex = 0;
  uint32_t bpp = 0;
  bool found = false;
  auto itFile = filesMap.end();

  const char* pExt = NULL;
  for (const string& ext : cExtensions)
  {
    itFile = filesMap.find(texture.mName + ext);
    if (itFile != filesMap.end())
    {
      found = true;
      pExt = ext.c_str() + 1;
      break;
    }
  }

  if (!found)
  {
    itFile = filesMap.find("transparent.png");
    if (itFile != filesMap.end())
    {
      found = true;
      pExt = cExtensions[2].c_str() + 1;
    }
  }

  if (found)
  {
    void* pFile = ReadFileAtPos(fileHandle, itFile->second);
    if (pFile != nullptr)
    {
      SDL_Surface* surface = IMG_LoadTyped_RW(SDL_RWFromConstMem(pFile, itFile->second.second), 0, pExt);
      free(pFile); //ToDo: avoid copying
      if (surface != nullptr)
      {
        bpp = surface->format->BytesPerPixel;
        tex = LoadTexture(surface);
      }
      else
      {
        std::cout << "Couldn't Load " << texture.mName << std::endl;
      }
    }
  }
  else
  {
    std::cout << "Couldn't Find " << texture.mName << std::endl;
  }

  uint32_t texIx = textures.size();

  textures.push_back(tex);

  string texName = toLower(texture.mName);
  if (bpp == 4)
  {
    SetTexType(texIx, ETexTypeTransparent, texturesTypeBits);
  }
  else if (texName.find("flame") != string::npos)
  {
    SetTexType(texIx, ETexTypeFlame, texturesTypeBits);
  }
  else if (texName.find("swirl") != string::npos)
  {
    SetTexType(texIx, ETexTypeSwirl, texturesTypeBits);
  }
}

Q3Map::Q3Map(const std::string& mapZipPath)
{
  unzFile mapFileHandle = unzOpen(mapZipPath.c_str());
  TFilePosAndLen bspFilePosAndLen;
  TArchivedFilesInfoMap filesMap = ReadArchiveFileNames(mapFileHandle, bspFilePosAndLen);

  if (bspFilePosAndLen.second <= 0)
  {
    std::cout << "Couldn't read map file: " << mapZipPath << std::endl;
    return;
  }

  void* pFile = ReadFileAtPos(mapFileHandle, bspFilePosAndLen);

  // ToDo: Optimization - avoid the copying of the file buffer
  std::stringstream localStream(string((char*)pFile, (char*)pFile + bspFilePosAndLen.second));
  //localStream.rdbuf()->pubsetbuf((char*)pFile, bspFilePosAndLen.second);

  free(pFile);

  if (!readMap(localStream, mMap, 0.03f, PostProcess_CoordSysOpenGL|PostProcess_FlipWindingOrder|PostProcess_TriangulateBezierPatches))
  {
    return;
  }

  uint32_t bpp = 0;

  // Load textures
  mTextures.reserve(mMap.mTextures.size());
  mTexturesTypeBits.assign(mMap.mTextures.size() / 32 + 1, 0ULL);
  for (const TTexture& texture : mMap.mTextures) 
  {
    LoadTexture(texture, mapFileHandle, filesMap, mTextures, mTexturesTypeBits);
  }

  unzClose(mapFileHandle);

  // Load lightmaps
  mLightMaps.reserve(mMap.mLightMaps.size());
  for (const TLightMap& lightMap : mMap.mLightMaps) 
  {
    mLightMaps.push_back(LoadTexture((const void *)lightMap.mMapData, 128));
  }

  // Update flame quads to use rotating billboards facing towards the camera
  UpdateFlameQuads();

  // Init OpenGL buffers
  InitBuffers(); 
}

Q3Map::~Q3Map() 
{
  glDeleteVertexArrays(1, &mVao);
  glDeleteBuffers(mBufferObjects.size(), mBufferObjects.data());
  glDeleteTextures(mTextures.size(), mTextures.data());
  glDeleteTextures(mLightMaps.size(), mLightMaps.data());
}

void Q3Map::UpdateFlameQuads()
{
  for (TFace& face : mMap.mFaces)
  {
    if (GetTexType(face.mTextureIndex, mTexturesTypeBits) == ETexTypeFlame)
    {
      vec3 p[4];
      for (unsigned i = 0; i < 4; i++)
      {
        int index = face.mVertex + mMap.mMeshVertices[face.mMeshVertex + i];
        p[i] = make_vec3(mMap.mVertices[index].mPosition);
      }

      vec3 origin = p[0] + (p[1] - p[0]) * 0.5f;
      float halfHeight = length(p[2] - p[1]) * 0.5f;
      float halfWidth = length(p[2] - p[0]) * 0.5f;
      if (halfWidth > halfHeight)
      {
        swap(halfWidth, halfHeight);
      }

      // ToDo: remove quick and dirty fix
      if (face.mTextureIndex == 60)
      {
        halfHeight *= 1.3f; // Some of the flame quads are smaller then others
      }

      // Generate the flame quads
      p[0] = origin + vec3(-halfWidth, +halfHeight, 0.f);
      p[1] = origin + vec3(+halfWidth, -halfHeight, 0.f);
      p[2] = origin + vec3(-halfWidth, -halfHeight, 0.f);
      p[3] = origin + vec3(+halfWidth, +halfHeight, 0.f);

      face.mNbVertices = 4;
      face.mNbMeshVertices = 6;
      face.mMeshVertex = 6;

      TVertex* v = &mMap.mVertices[face.mVertex];
      float newTex[4][2] = { { 1.f, 0.f },{ 0.f, 1.f },{ 1.f, 1.f },{ 0.f, 0.f } };
      for (unsigned i = 0; i < 4; i++)
      {
        memcpy(v[i].mPosition, &p[i], 12);
        memcpy(v[i].mTexCoord[0], newTex[i], 8);
      }
    }
  }
}

void Q3Map::InitBuffers() 
{
  GLuint buffer = 0;

  // Generate Vertex Array Object
  glGenVertexArrays(1, &mVao);
  glBindVertexArray(mVao);

  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(TMeshVert) * mMap.mMeshVertices.size(), mMap.mMeshVertices.data(), GL_STATIC_DRAW);
  mBufferObjects.push_back(buffer);

  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(TVertex) * mMap.mVertices.size(), mMap.mVertices.data(), GL_STATIC_DRAW);
  mBufferObjects.push_back(buffer);

  glEnableVertexAttribArray(VERT_POSITION_LOC);
  glVertexAttribPointer(VERT_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(TVertex), 0);

  glEnableVertexAttribArray(VERT_DIFFUSE_TEX_COORD_LOC);
  glVertexAttribPointer(VERT_DIFFUSE_TEX_COORD_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(TVertex), (void *)12);

  glEnableVertexAttribArray(VERT_LIGHTMAP_TEX_COORD_LOC);
  glVertexAttribPointer(VERT_LIGHTMAP_TEX_COORD_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(TVertex), (void *)20);

  glEnableVertexAttribArray(VERT_NORMAL_LOC);
  glVertexAttribPointer(VERT_NORMAL_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(TVertex), (void *)28);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void Q3Map::Render(
  const Resources& resources, 
  const mat4& viewMat, 
  const mat4& projMat, 
  const vec3& camPos,
  uint32 matUniformBuffer) const
{
  VisibleFacesByType faces = FindVisibleFaces(camPos, projMat * viewMat);
  if (faces.empty())
  {
    return;
  }

  // Set the tesselation inner and outer levels for the patches
  vec4 tessOuterLvl(10.0f);
  vec2 tessInnerLvl(10.0f);
  glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, value_ptr(tessOuterLvl));
  glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, value_ptr(tessInnerLvl));
  glPatchParameteri(GL_PATCH_VERTICES, 9);

  // Enable blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glPointSize(5.0f);
  glLineWidth(5.f);
  glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEX_UNIT);
  glActiveTexture(GL_TEXTURE0 + LIGHTMAP_TEX_UNIT);
  glUseProgram(resources.GetProgram("simple"));

  glBindVertexArray(mVao);

  for (auto pair : faces[EFaceTypeBsp1]) 
  {
    RenderFace(pair.first);
  }

  for (auto pair : faces[EFaceTypeBsp3]) 
  {
    RenderFace(pair.first);
  }
  
  for (auto pair : faces[EFaceTypeBsp2]) 
  {
    RenderFace(pair.first);
  }
  
  glDisable(GL_CULL_FACE);

  glUseProgram(resources.GetProgram("flame"));

  float time = SDL_GetTicks() * 0.001f;
  
  glUniform3fv(CAMERA_POS_LOC, 1, value_ptr(camPos));
  glUniform3fv(BILLBOARD_ROTATION_AXIS, 1, value_ptr(vec3(0.f, 1.f, 0.f)));
  glUniform1i(BILLBOARD_IN_WORLD_SPACE, GL_TRUE);

  for (auto pair : faces[EFaceTypeFlame]) 
  {
    const TFace& face = mMap.mFaces[pair.first];

    vec3 p1 = make_vec3(mMap.mVertices[face.mVertex + 1].mPosition);
    vec3 p2 = make_vec3(mMap.mVertices[face.mVertex + 2].mPosition);
    vec3 c = p1 + (p2 - p1) * 0.5f;

    glUniform1f(GLOBAL_TIME_LOC, time + 0.3345f * pair.first);
    glUniform3fv(BILLBOARD_ORIGIN_LOC, 1, value_ptr(c));

    RenderFace(pair.first);
  }

  glUseProgram(resources.GetProgram("swirl"));

  for (auto pair : faces[EFaceTypeSwirl]) 
  {
    const TFace& face = mMap.mFaces[pair.first];

    glUniform1f(GLOBAL_TIME_LOC, time + 0.3345f * pair.first);

    RenderFace(pair.first);
  }

  glUseProgram(resources.GetProgram("simple"));
  glBindVertexArray(mVao);

  for (auto pair : faces[EFaceTypeTransparent]) 
  {
    RenderFace(pair.first);
  }

  glDisable(GL_CULL_FACE);
}

void Q3Map::RenderFace(uint32_t faceIndex) const 
{
  const TFace& face = mMap.mFaces[faceIndex];

  static int32_t lastDiffuseTex = -1;
  if (face.mTextureIndex != lastDiffuseTex) 
  {
    glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEX_UNIT);
    if (face.mTextureIndex >= 0) 
    {
      glBindTexture(GL_TEXTURE_2D, mTextures[face.mTextureIndex]);
    } 
    else 
    {
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    lastDiffuseTex = face.mTextureIndex;
  }

  static int32_t lastLightmapTex = -1;
  if (face.mLightmapIndex != lastLightmapTex) 
  {
    glActiveTexture(GL_TEXTURE0 + LIGHTMAP_TEX_UNIT);
    if (face.mLightmapIndex >= 0) 
    {
      glBindTexture(GL_TEXTURE_2D, mLightMaps[face.mLightmapIndex]);
    } 
    else 
    {
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    lastLightmapTex = face.mLightmapIndex;
  }

  if (face.mType == 1 || face.mType == 3 || face.mType == 2) 
  {
    glDrawElementsBaseVertex(
      GL_TRIANGLES,
      face.mNbMeshVertices,
      GL_UNSIGNED_INT,
      (void *)(face.mMeshVertex * sizeof(uint32_t)),
      face.mVertex);
  }
  /*else if (face.mType == 2) 
  {
    glDrawElementsBaseVertex(
      GL_PATCHES,
      face.mNbMeshVertices,
      GL_UNSIGNED_INT,
      (void *)(face.mMeshVertex * sizeof(uint32_t)),
      face.mVertex);
  }*/
}

Q3Map::VisibleFacesByType Q3Map::FindVisibleFaces(const vec3& camPos, const mat4& mvpMat) const
{
  int leaf;
  int visCluster;
  VisibleFacesByType visibleFaces(EFaceTypeMax);
  vector<uint64_t> visFaceMask(mMap.mFaces.size() / 64 + 1, 0ULL);

  if (mMap.mLeaves.empty()) 
  { 
    return VisibleFacesByType(); 
  }

  leaf = FindLeaf(camPos);

  visCluster = mMap.mLeaves[leaf].mCluster;
  if (visCluster < 0)
  { 
    return VisibleFacesByType(); 
  }

  FrustumPlanes planes = CalcFrustumPlanes(mvpMat);

  for (uint32_t i = 0; i < mMap.mLeaves.size(); i++)
  {
    if (!IsClusterVisible(visCluster, mMap.mLeaves[i].mCluster)) 
    { 
      continue; 
    }

    vec3 minS = make_vec3(mMap.mLeaves[i].mMins);
    vec3 maxS = make_vec3(mMap.mLeaves[i].mMaxs);
    vec3 halfSize = (maxS - minS) * 0.5f;
    if (!IsBoxInFrustum(minS + halfSize, halfSize, planes))
    {
      continue;
    }

    for (int k = 0; k < mMap.mLeaves[i].mNbLeafFaces; k++)
    {
      uint32_t faceindex = mMap.mLeafFaces[mMap.mLeaves[i].mLeafFace + k].mFaceIndex;

      // Optimization: use 1 bit per face to minimize cache misses (3.55% => 0.2% perf improve)
      if (!IsBitSet(faceindex, visFaceMask))
      {
        SetBit(faceindex, visFaceMask);

        const TFace& face = mMap.mFaces[faceindex];
        uint32_t faceType = face.mType;
        assert(faceType > 0 && faceType < 5);
        switch (GetTexType(face.mTextureIndex, mTexturesTypeBits))
        {
        case ETexTypeTransparent:
          faceType = EFaceTypeTransparent; break;
        case ETexTypeFlame:
          faceType = EFaceTypeFlame; break;
        case ETexTypeSwirl:
          faceType = EFaceTypeSwirl; break;
        }

        visibleFaces[faceType].push_back(FaceIdAndHash(faceindex, GetFaceHash(face)));
      }
    }
  }

  // Sort visibleFaces to minimase the number glActiveTexture calls when iterated
  for (auto& faces : visibleFaces)
  {
    std::sort(begin(faces), end(faces), [](const FaceIdAndHash& f1, const FaceIdAndHash& f2) { return f1.second < f2.second; });
  }
  
  return visibleFaces;
}

int Q3Map::FindLeaf(const glm::vec3& camPos) const
{
  int index = 0;

  while (index >= 0)
  {
    const TNode& node = mMap.mNodes[index];
    const TPlane& plane = mMap.mPlanes[node.mPlane];

    // Distance from point to plane
    vec3 normal = make_vec3(plane.mNormal);
    const float distance = dot(normal, camPos) - plane.mDistance;

    if (distance >= 0)
    {
      index = node.mChildren[0]; // Front
    }
    else
    {
      index = node.mChildren[1]; // Back
    }
  }

  return -index - 1;
}

bool Q3Map::IsClusterVisible(int visCluster, int testCluster) const
{
  if (mMap.mVisData.mBuffer.empty())
  {
    return true;
  }

  if ((visCluster < 0) || (testCluster < 0))
  {
    return true;
  }

  int i = (visCluster * mMap.mVisData.mBytesPerCluster) + (testCluster >> 3);
  unsigned char visSet = mMap.mVisData.mBuffer[i];

  return (visSet & (1 << (testCluster & 7))) != 0;
}

TraceData::TraceData(const vec3& start, const vec3& end)
  : mTraceType(TracePoint), mStart(start), mEnd(end), mRadius(0.f)
  , mPlaneIndex(-1), mFraction(1.f), mStartsOut(true), mAllSolid(false), mCollision(false)
{}

TraceData::TraceData(const vec3& start, const vec3& end, float radius)
  : mTraceType(TraceSphere), mStart(start), mEnd(end), mRadius(radius)
  , mPlaneIndex(-1), mFraction(1.f), mStartsOut(true), mAllSolid(false), mCollision(false)
{}

TraceData::TraceData(const vec3& start, const vec3& end, const vec3& minBounds, const vec3& maxBounds)
  : mTraceType(TraceBox), mStart(start), mEnd(end), mRadius(0.f)
  , mMinBounds(minBounds), mMaxBounds(maxBounds), mExtends(max(-minBounds, maxBounds))
  , mPlaneIndex(-1), mFraction(1.f), mStartsOut(true), mAllSolid(false), mCollision(false)
{
  assert(sign(mExtends) == vec3(1.f));
}

bool Q3Map::Trace(TraceData& data) const
{
  CheckNode(0, data);

  if (data.mCollision)
  {
    vec3 innerBrushVec = (data.mEnd - data.mStart) * (1.f - data.mFraction);
    const TPlane& plane = mMap.mPlanes[data.mPlaneIndex];
    data.mPlaneProj = proj(make_vec3(plane.mNormal), innerBrushVec);
  }

  return data.mCollision;
}

void Q3Map::GetVerticesAndIndices(std::vector<float>& outVertices, std::vector<float>& outNormals, std::vector<int>& outIndices)
{
  const int maxVertices = mMap.mVertices.size();
  const int maxIndices = mMap.mMeshVertices.size();

  outVertices.reserve(maxVertices * 3);
  outNormals.reserve(maxVertices * 3);
  for (const TVertex& vertex : mMap.mVertices)
  {
    outVertices.push_back(vertex.mPosition[0]);
    outVertices.push_back(vertex.mPosition[1]);
    outVertices.push_back(vertex.mPosition[2]);

    outNormals.push_back(vertex.mNormal[0]);
    outNormals.push_back(vertex.mNormal[1]);
    outNormals.push_back(vertex.mNormal[2]);
  }

  outIndices.reserve(maxIndices);
  for (const TFace& face : mMap.mFaces)
  {
    int contents = mMap.mTextures[face.mTextureIndex].mContents;
    if ((face.mType == 4) || ((contents & CONTENTS_SOLID) == 0)) continue;

    for (int i = 0; i < face.mNbMeshVertices; i++)
    {
      outIndices.push_back(face.mVertex + mMap.mMeshVertices[face.mMeshVertex + i]);
    }
  }
}

void Q3Map::CheckNode(int nodeIndex, TraceData& data) const
{
  if (nodeIndex < 0)
  {   // this is a leaf
    const uint32_t leafIndex = -(nodeIndex + 1);
    CheckLeaf(leafIndex, data);
    return;
  }

  const TNode& node = mMap.mNodes[nodeIndex];
  const TPlane& plane = mMap.mPlanes[node.mPlane];
  vec3 planeNormal = make_vec3(plane.mNormal);

  float offset = data.mRadius;
  if (data.mTraceType == TraceData::TraceBox)
  {
    offset = dot(data.mExtends, abs(planeNormal));
  } 

  float startDistance = dot(data.mStart, planeNormal) - plane.mDistance;
  float endDistance = dot(data.mEnd, planeNormal) - plane.mDistance;

  if (startDistance >= offset && endDistance >= offset)
  {   // both points are in front of the plane
    // so check the front child
    CheckNode(node.mChildren[0], data);
  }
  else if (startDistance < -offset && endDistance < -offset)
  {   // both points are behind the plane
    // so check the back child
    CheckNode(node.mChildren[1], data);
  }
  else
  {   // the line spans the splitting plane
    int side = (startDistance < endDistance ? 1 : 0);

    // Check the first side
    CheckNode(node.mChildren[side], data);

    // Check the second side
    CheckNode(node.mChildren[!side], data);
  }
}

void Q3Map::CheckLeaf(int leafIndex, TraceData& data) const
{
  const TLeaf& leaf = mMap.mLeaves[leafIndex];

  for (int i = 0; i < leaf.mNbLeafBrushes; i++)
  {
    int brushIndex = mMap.mLeafBrushes[leaf.mLeafBrush + i].mBrushIndex;

    // avoid checking the same brush more then once
    if (data.mCheckedBrushes.find(brushIndex) != end(data.mCheckedBrushes)) { continue; }

    data.mCheckedBrushes.insert(brushIndex);

    CheckBrush(brushIndex, data);
  }
}

void Q3Map::CheckBrush(int brushIndex, TraceData& data) const
{
  const TBrush& brush = mMap.mBrushes[brushIndex];
  int contents = mMap.mTextures[brush.mTextureIndex].mContents;
  if (brush.mNbBrushSides <= 0 || ((contents & CONTENTS_SOLID) == 0))
  {
    return;
  }

  float enterFraction = -1.f, leaveFraction = 1.f;
  int32_t startPlaneIndex = -1;
  bool startsOut = false, endsOut = false;

  for (int i = 0; i < brush.mNbBrushSides; i++)
  {
    const TBrushSide& brushSide = mMap.mBrushSides[brush.mBrushSide + i];
    const TPlane& plane = mMap.mPlanes[brushSide.mPlaneIndex];
    vec3 planeNormal = normalize(make_vec3(plane.mNormal));

    float dist = data.mRadius;
    if (data.mTraceType == TraceData::TraceBox)
    {
      vec3 offset;
      for (int j = 0; j < 3; j++)
      {
        if (planeNormal[j] < 0.f)
        {
          offset[j] = data.mMaxBounds[j];
        }
        else
        {
          offset[j] = data.mMinBounds[j];
        }
      }

      dist = -dot(offset, planeNormal);
    }

    float startDistance = dot(data.mStart, planeNormal) - (plane.mDistance + dist);
    float endDistance = dot(data.mEnd, planeNormal) - (plane.mDistance + dist);

    if (startDistance > 0.f) { startsOut = true; }
    if (endDistance > 0.f) { endsOut = true; }

    // make sure the trace isn't completely on one side of the brush
    if (startDistance > 0.f && endDistance >= startDistance)
    {   // if completely in front of face, no intersection
      return;
    }
    if (startDistance <= 0.f && endDistance <= 0.f)
    {   // both are behind this plane, it will get clipped by another one
      continue;
    }

    // crosses face
    if (startDistance > endDistance)
    {   // line is entering into the brush
      float fraction = (startDistance - 0.0001f) / (startDistance - endDistance);
      if (fraction > enterFraction)
      {
        enterFraction = fraction;
        startPlaneIndex = brushSide.mPlaneIndex;
      }
    }
    else
    {   // line is leaving the brush
      float fraction = (startDistance + 0.0001f) / (startDistance - endDistance);
      if (fraction < leaveFraction)
      {
        leaveFraction = fraction;
      }
    }
  }

  if (startsOut == false)
  {
    data.mStartsOut = false;
    if (endsOut == false)
    {
      data.mAllSolid = true;
    }
    return;
  }

  if (enterFraction < leaveFraction)
  {
    if (enterFraction > -1 && enterFraction <= data.mFraction)
    {
      if (enterFraction < 0.f) {
        enterFraction = 0;
      }

      assert(startPlaneIndex >= 0);
      data.mPlaneIndex = startPlaneIndex;
      data.mFraction = enterFraction;
      data.mContents = contents;
      data.mCollision = true;
    }
  }
}
