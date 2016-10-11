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

#include "components.hpp"
#include "camera_utils.hpp"
#include "Q3Map.hpp"

#include <iostream>

#include <glm/gtx/euler_angles.hpp>

using namespace glm;

namespace shooter {

  inline int VectorToIndex(const glm::vec3 &v)
  {
    int idx = 0;
    if (v.z >= 0) idx |= 1;
    if (v.y >= 0) idx |= 2;
    if (v.x >= 0) idx |= 4;
    return idx;
  }

  enum PointPlaneTest
  {
    EPointBehindPlane = 0,
    EPointInFrontOfPlane,
    EPointOnPlane
  };

  inline int HalfPlaneTest(const glm::vec3& p, const glm::vec3& normal, float offset)
  {
    float dist = glm::dot(p, normal) + offset;

    if (dist > 0.001f) 
    { 
      return EPointInFrontOfPlane; 
    }
    else if (dist < -0.001f) 
    { 
      return EPointBehindPlane; 
    }

    return EPointOnPlane;
  }

  int IsBoxInFrustum(const glm::vec3 &origin, const glm::vec3 &halfDim, const FrustumPlanes& planes)
  {
    static const glm::vec3 cornerOffsets[] = {
      glm::vec3(-1.f, -1.f, -1.f),
      glm::vec3(-1.f, -1.f, 1.f),
      glm::vec3(-1.f, 1.f, -1.f),
      glm::vec3(-1.f, 1.f, 1.f),
      glm::vec3(1.f, -1.f, -1.f),
      glm::vec3(1.f, -1.f, 1.f),
      glm::vec3(1.f, 1.f, -1.f),
      glm::vec3(1.f, 1.f, 1.f)
    };

    int ret = 1;
    for (int i = 0; i < 6; i++)
    {
      glm::vec3 planeNormal = glm::vec3(planes[i]);
      int idx = VectorToIndex(planeNormal);

      // Test the farthest point of the box from the plane
      // if it's behind the plane, then the entire box will be.
      glm::vec3 testPoint = origin + halfDim * cornerOffsets[idx];

      if (HalfPlaneTest(testPoint, planeNormal, planes[i].w) == EPointBehindPlane)
      {
        ret = 0;
        break;
      }

      // Now, test the closest point to the plane
      // If it's behind the plane, then the box is partially inside, otherwise it is entirely inside.
      idx = VectorToIndex(-planeNormal);
      testPoint = origin + halfDim * cornerOffsets[idx];

      if (HalfPlaneTest(testPoint, planeNormal, planes[i].w) == 0)
      {
        ret |= 2;
      }
    }

    return ret;
  }

  FrustumPlanes CalcFrustumPlanes(const glm::mat4& matrix)
  {
    std::vector<glm::vec4> planes(6);

    // Extract frustum planes from matrix
    // Planes are in format: normal(xyz), offset(w)
    planes[ERightFrustumPlane] = glm::vec4(
      matrix[0][3] - matrix[0][0],
      matrix[1][3] - matrix[1][0],
      matrix[2][3] - matrix[2][0],
      matrix[3][3] - matrix[3][0]);

    planes[ELeftFrustumPlane] = glm::vec4(
      matrix[0][3] + matrix[0][0],
      matrix[1][3] + matrix[1][0],
      matrix[2][3] + matrix[2][0],
      matrix[3][3] + matrix[3][0]);

    planes[EBottomFrustumPlane] = glm::vec4(
      matrix[0][3] + matrix[0][1],
      matrix[1][3] + matrix[1][1],
      matrix[2][3] + matrix[2][1],
      matrix[3][3] + matrix[3][1]);

    planes[ETopFrustumPlane] = glm::vec4(
      matrix[0][3] - matrix[0][1],
      matrix[1][3] - matrix[1][1],
      matrix[2][3] - matrix[2][1],
      matrix[3][3] - matrix[3][1]);

    planes[EFarFrustumPlane] = glm::vec4(
      matrix[0][3] - matrix[0][2],
      matrix[1][3] - matrix[1][2],
      matrix[2][3] - matrix[2][2],
      matrix[3][3] - matrix[3][2]);

    planes[ENearFrustumPlane] = glm::vec4(
      matrix[0][3] + matrix[0][2],
      matrix[1][3] + matrix[1][2],
      matrix[2][3] + matrix[2][2],
      matrix[3][3] + matrix[3][2]);
    
    // Normalize them
    for (int i = 0; i < 6; i++)
    {
      float invl = sqrt(
        planes[i].x * planes[i].x +
        planes[i].y * planes[i].y +
        planes[i].z * planes[i].z);

      planes[i] /= invl;
    }

    return planes;
  }

  glm::mat4 CalcProjMat(const CompFrustum& frustum)
  {
    return perspective(radians(frustum.fov), frustum.aspect_ratio, frustum.near, frustum.far);
  }

  glm::mat4 CalcViewMat(const CompTransform& trans)
  {
    return lookAt(trans.position, trans.position + trans.front, vec3(0.f, 1.f, 0.f));
  }

  glm::mat4 CalcTransMat(const CompTransform& trans)
  {
    const glm::vec3 worldUp(0.f, 1.f, 0.f);
    const glm::vec3 right = glm::normalize(glm::cross(-trans.front, worldUp));
    return glm::translate(trans.position)
      * glm::mat4(glm::mat3(right, normalize(glm::cross(trans.front, right)), -trans.front))
      * glm::scale(glm::vec3(trans.scale));
  }
}
