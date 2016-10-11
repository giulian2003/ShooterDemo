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

#ifndef Q3MAP_HPP
#define Q3MAP_HPP

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <GL/glew.h>

#include "Q3Loader.h"

namespace shooter {

  struct CompCamera;
  class Resources;
  class NavMesh;

  /// Data describing the intersection of a moving point/shape with the Q3 map
  struct TraceData
  {
    enum TraceType 
    {
      TracePoint = 0,
      TraceSphere,
      TraceBox
    };

    /// Constructor for tracing a moving point (ray cast)
    TraceData(const glm::vec3& start, const glm::vec3& end);
    
    /// Constructor for tracing a moving sphere
    TraceData(const glm::vec3& start, const glm::vec3& end, float radius);

    /// Constructor for tracing a moving box
    TraceData(const glm::vec3& start, const glm::vec3& end, const glm::vec3& minBounds, const glm::vec3& maxBounds);

    /// Input data (set in constructors)
    const TraceType mTraceType; ///< Volume type
    const glm::vec3 mStart; ///< Start position
    const glm::vec3 mEnd; ///< End position
    const float mRadius; ///< Radius (in case of TraceSphere)

    const glm::vec3 mMinBounds; ///< Shape minimum bounds position
    const glm::vec3 mMaxBounds; ///< Shape maximum bounds position
    const glm::vec3 mExtends; ///< Shape extends on X,Y and Z axii

    /// Output data
    bool mCollision; ///< True if there was a collision with the map, false otherwise
    bool mStartsOut; ///< True if the trace starts outside a Q3 Map Brush, false otherwise
    bool mAllSolid; ///< True if the trace starts and ends outside a Q3 Map Brush, false otherwise

    int32_t mPlaneIndex; ///< Index of closest intersected plane or -1 if no collision
    float mFraction; ///< Describes the intersection point which is (mEnd - mStart) * mFraction
    int mContents; ///< Contents Flag of the intersected surface
    glm::vec3 mPlaneProj; ///< Projection of the ray on the intersecting plane 

    std::unordered_set<uint32_t> mCheckedBrushes; ///< indices of Brushes checked 
  };

  /// Class describing the map. Contains methods to do ray/volume tracing and rendering of the map.
  class Q3Map
  {
  public:
    Q3Map(const std::string& mapZipPath);
    ~Q3Map();

    const TMapQ3& GetMapQ3() const { return mMap; }

    /// Render the map
    void Render(
      const Resources& resources,
      const glm::mat4& viewMat,
      const glm::mat4& projMat,
      const glm::vec3& camPos,
      uint32_t matUniformBuffer) const;

    /// Trace a moving point or volume intersection with the map 
    bool Trace(TraceData& data) const;

    /// Get the unindexed vertices, normals and indices
    void GetVerticesAndIndices(std::vector<float>& outVertices, std::vector<float>& outNormals, std::vector<int>& outIndices);

  private:
    enum FaceType
    {
      EFaceTypeTransparent = 0,
      EFaceTypeBsp1,
      EFaceTypeBsp2,
      EFaceTypeBsp3,
      EFaceTypeBsp4,
      EFaceTypeFlame,
      EFaceTypeSwirl,
      EFaceTypeMax
    };

    typedef std::pair<uint32_t, uint32_t> FaceIdAndHash;
    typedef std::vector<std::vector<FaceIdAndHash> > VisibleFacesByType;

    /// Update the flame vertices and indexes in the mMap to use rotating billboards
    void UpdateFlameQuads();

    /// Init OpenGL buffers needed for rendering
    void InitBuffers();

    /// Render a face
    void RenderFace(uint32_t faceIndex) const;

    /// Find all visible faces
    VisibleFacesByType FindVisibleFaces(const glm::vec3& camPos, const glm::mat4& mvpMat) const;

    /// Functions inspired from (http://graphics.cs.brown.edu/games/quake/quake3.html)
    int FindLeaf(const glm::vec3& camPos) const;
    bool IsClusterVisible(int visCluster, int testCluster) const;

    // Recursive functions called by Trace()
    void CheckNode(int nodeIndex, TraceData& data) const;
    void CheckLeaf(int leafIndex, TraceData& data) const;
    void CheckBrush(int brushIndex, TraceData& data) const;

    TMapQ3 mMap;
    std::vector<GLuint> mLightMaps; ///< OpenGL texture objs for the Light Map Textures
    std::vector<GLuint> mTextures; ///< OpenGL texture objs for the Diffuse Textures
    std::vector<uint64_t> mTexturesTypeBits; ///< Bit Mask containing the texture type for each texture in mTextures (2 bit/texture)
    
    GLuint mVao; ///< OpenGL vertex array object of the map
    std::vector<GLuint> mBufferObjects; ///< OpenGL buffer objs
  };
}

#endif // Q3MAP_HPP