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

#ifndef RESOURCES_HPP
#define RESOURCES_HPP

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GL/glew.h>

#include <assimp/anim.h>

#include "Q3Map.hpp"
#include "nav_mesh.hpp"

struct aiScene;
struct aiNode;
struct aiAnimation;
struct aiVertexWeight;
struct aiMesh;

namespace shooter {

  const uint32_t cMatrixSize = sizeof(glm::mat4);
  const uint32_t cNormalMatrixRowSize = 4 * sizeof(float);
  const uint32_t cMatricesUniBufferSize = cMatrixSize * 4;
  const uint32_t cProjMatrixOffset = 0;
  const uint32_t cViewMatrixOffset = cMatrixSize;
  const uint32_t cModelMatrixOffset = cMatrixSize * 2;
  const uint32_t cNormalMatrixOffset = cMatrixSize * 3;

  /// Contains data needed to render each mesh
  struct Mesh  
  {
    GLuint vao; ///< OpenGL vertex array object 
    uint32_t materialIndex; ///< Index of the material used for this mesh
    uint32_t numFaces; ///< Number of triangles this mesh contains
  };

  /// Contains data needed to animate each vertex in the GL Vertex Shader
  struct VertexBoneData 
  {
    glm::uvec4 boneIds; ///< All bones that affect this vertex (max 4)
    glm::vec4 weights; ///< Weights for each bone
  };

  /// Describes each animation frame
  struct AnimationFrame 
  {
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scaling;
  };

  /// Describes the animation behavior
  enum AnimBehaviour 
  {
    EAnimBehaviourDefault = aiAnimBehaviour_DEFAULT,    ///< The value from the default node transformation is taken
    EAnimBehaviourConstant = aiAnimBehaviour_CONSTANT,  ///< The nearest key value is used without interpolation
    EAnimBehaviourLinear = aiAnimBehaviour_LINEAR,      ///< The value of the nearest two keys is linearly extrapolated for the current time value
    EAnimBehaviourRepeat = aiAnimBehaviour_REPEAT,      ///< The animation is repeated
  };

  /// Describes the animation frames of one node
  struct NodeAnimation 
  {
    AnimBehaviour preState;
    AnimBehaviour postState;
    std::vector<std::pair<float, glm::vec3> > translations;
    std::vector<std::pair<float, glm::quat> > rotations;
    std::vector<std::pair<float, glm::vec3> > scalings;
  };
  
  /// Describes the animation of all nodes
  struct Animation 
  {
    Animation() 
      : durationInTicks(0.f)
      , ticksPerSecond(0.f) 
    {}

    float durationInTicks;
    float ticksPerSecond;
    std::vector<NodeAnimation> nodesAnimation;
  };

  /// Data used in OpenGL shader uniform block
  struct MaterialColors
  {
    MaterialColors() 
      : shininess(0.f)
      , texCount(0) 
    {}

    glm::vec4 diffuse;
    glm::vec4 ambient;
    glm::vec4 specular;
    glm::vec4 emissive;
    float shininess;
    uint32_t texCount; ///< Number of textures
  };

  enum TextureType 
  {
    ETextureType_NONE = 0x0,
    ETextureType_DIFFUSE = 0x1,
    ETextureType_SPECULAR = 0x2,
    ETextureType_AMBIENT = 0x3,
    ETextureType_EMISSIVE = 0x4,
    ETextureType_HEIGHT = 0x5,
    ETextureType_NORMALS = 0x6,
    ETextureType_SHININESS = 0x7,
    ETextureType_OPACITY = 0x8,
    ETextureType_DISPLACEMENT = 0x9,
    ETextureType_LIGHTMAP = 0xA,
    ETextureType_REFLECTION = 0xB,
    ETextureType_MAX = 0xC,
  };

  typedef std::pair<TextureType, GLuint> TextureTypeAndObj; /// Material texture type and OpenGL obj
  typedef std::vector<TextureTypeAndObj> MaterialTextures; /// Collection of textures needed by a certain material 

  /// Describes the Light (used in OpenGL shader uniform block)
  struct Light 
  {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
  };

  typedef std::unordered_map<std::string, uint32_t> NamesAndIdsMap;

  /// Describes a 3D model. Contains all the data needed to render and animate it.
  /// Data is stored as Structure of Arrays.
  struct Model 
  {
    /// Node animation data. 
    /// For performance reasons, I store the animation data as a Structure of Arrays indexed by the Node Index.
    /// The Node Indices are calculated traversing the assimp animation tree in pre-order.
    /// Advantages of this approach:
    ///  1. Data is stored in a continuous memory area. (easier to copy and less cache misses)
    ///  2. To calculate the transformation matrices of the animation, we can simply
    ///     iterate the arrays sequentially because IndexParentNode < IndexNode.
    ///
    /// Assimp uses Bones as a sub-set of Nodes, but for simplification, 
    /// we use as many bones as we have nodes, so BoneIndex == NodeIndex
    ///
    NamesAndIdsMap nodesMap; ///< Map of Node names and indices

    std::vector<int16_t> nodesParents; // The ParentIndex for each NodeIndex

    glm::mat4 globalInvTrans; ///< Inverse of ai_scene::mRootNode::mTransformation
    std::vector<glm::mat4> nodesTrans; ///< Transforms from the Node Space, into the Node's Parent Space (assimp_node::mTransformation)
    std::vector<glm::mat4> bonesOffsets; ///< Transforms from the Mesh Local Space, into the Bone Space (assimp_mesh::mBones[i]::mOffsetMatrix)
    std::vector<glm::mat4> invBonesOffsets; ///< Precalculated inverse matrices for bonesOffsets

    std::unordered_map<std::string, Animation> animationsMap; ///< Map with all the animations available for this model

    /// Rendering data 
    NamesAndIdsMap textureMap; ///< Map of Texture names and indices
    std::vector<Mesh> meshes;  ///< Collection of meshes for this model
    std::vector<GLuint> textures; ///< OpenGL texture objects, indexed by Texture Index
    
    std::vector<MaterialTextures> materialsTex; ///< Material textures, indexed by Material Index
    std::vector<GLuint> materialsCol; ///< Material colors, indexed by Material Index

    glm::vec3 minBound, maxBound; ///< Minimum and maximum bound of the model
    float normScale; ///< Normalized scale (multiplied by this, the scale of the model becomes 1.0f)
  };

  typedef std::unordered_map<std::string, Model> ModelsMap;
  typedef std::unordered_map<std::string, GLuint> ProgramsMap;
  typedef std::unordered_map<std::string, GLuint> TextureMap;

  /// Manages all resources used by other Systems (Map, Models, shaders, etc.)
  class Resources
  {
  public:

    Resources(const std::string& resourcePath)
      : mResourceFolder(resourcePath)
      , mSkyBoxTexture(0)
    {}

    ~Resources();

    /// All folders are relative to resourcePath
    bool LoadPrograms(const std::string& folderPath); /// Load all shaders from folderPath
    bool LoadSkyBox(const std::string& folderPath); /// Load the skybox texture from folderPath
    bool LoadMap(const std::string& zipFilePath); /// Load the Q3Map from zipFilePath
    bool LoadModel(const std::string& filePath); /// Load the Player/NPC 3D model from filePath
    
    /// Accessors
    GLuint GetProgram(const std::string& programName) const;
    GLuint GetSkyBoxTexture() const { return mSkyBoxTexture; }
    const Q3Map& GetMap() const { return *mMap; }
    const NavMesh& GetNavMesh() const { return *mNavMesh; }
    const ModelsMap& GetModels() const { return mModels; }
    const Model& GetModel(const std::string& modelName) const;
    const Animation& GetAnimation(const Model& model, const std::string& animationName) const;
    const Animation& GetAnimation(const std::string& modelName, const std::string& animationName) const;

    /// Get the global transformation matrices for the current animation
    void GetSkeletonTransforms(
      const Model& model,
      const std::string& animationName,
      float animationTimeInSeconds, /// Current animation time
      float& inoutLastAnimationTimeInSeconds, /// [in] Previous animation time, [out] Current animation time
      std::vector<AnimationFrame>& inoutLastAnimationFrames, /// [in] Previous animation frames, [out] Current animation frames)
      std::vector<glm::mat4>& outGlobalTransforms /// [out] global transformation matrices for all the nodes
    ) const;

  private:
    /// Functions needed to read a Model from aiScene

    static const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string& NodeName);

    static void ProcessNodeAnim(const std::string& nodeName, const aiAnimation* anim, Animation& animation);

    static void ProcessNode(const aiScene* scene, const aiNode* pNode, Model& model, int32_t parentNodeIndex, int32_t nodeIndex);

    static void ProcessNodeHierarchy(const aiScene* scene, const aiNode* pNode, Model& model, int32_t parentNodeIndex);

    static GLuint LoadTexture(std::string name, TextureMap& textureMap);

    static void LoadEmbeddedTextures(const aiScene* scene, Model& model);

    static void LoadMaterials(const aiScene* scene, const std::string& altPath, Model& model);

    static std::vector<VertexBoneData> LoadBones(
      const aiMesh* pMesh, 
      const NamesAndIdsMap& nodesMap, 
      std::vector<glm::mat4>& bonesOffset, 
      std::vector<glm::mat4>& invBonesOffset);

    static void LoadMeshes(
      const aiScene* scene, 
      const std::string& modelPath, 
      Model& model, 
      std::vector<GLuint>& inoutBufferObjs);

    std::string mResourceFolder; ///< Path to the resource folder

    std::unique_ptr<Q3Map> mMap; ///< Q3 Map
    std::unique_ptr<NavMesh> mNavMesh; ///< Navigation Mesh

    ProgramsMap mPrograms; ///< Map of Program names and OpenGL objs
    ModelsMap mModels; ///< Map of Model names and objs

    GLuint mSkyBoxTexture; ///< Skybox OpenGL texture obj 
    std::vector<uint32_t> mBufferObjects; ///< OpenGL buffer objs refferences by the Meshes vertex array objs

    const Model mEmptyModel;
    const Animation mEmptyAnimation;
  };

}

#endif //RESOURCES_HPP