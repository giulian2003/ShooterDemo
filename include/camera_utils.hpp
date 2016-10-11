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

#ifndef CAMERA_UTILS_HPP
#define CAMERA_UTILS_HPP

#include <vector>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace shooter {

  struct CompTransform;
  struct CompFrustum;

  /// Enumates frustum planes.
  enum {
    ERightFrustumPlane = 0,
    ELeftFrustumPlane,
    EBottomFrustumPlane,
    ETopFrustumPlane,
    EFarFrustumPlane,
    ENearFrustumPlane
  };

  typedef std::vector<glm::vec4> FrustumPlanes;

  /// Calculate frustum planes from the Model-View-Projection matrix.
  FrustumPlanes CalcFrustumPlanes(
    const glm::mat4& mvpMatrix ///< Model-View-Projection matrix.
  );

  /// Checks if a box is inside the frustum.
  /// Based on: http://www.flipcode.com/archives/Octrees_For_Visibility.shtml 
  int IsBoxInFrustum(
    const glm::vec3& origin, ///< Box origin
    const glm::vec3& halfDim, ///< Box half diagonal
    const FrustumPlanes& planes ///< Frustum planes
  );

  /// Calculate the View matrix.
  glm::mat4 CalcViewMat(const CompTransform& trans);
  
  /// Calculate the Projection matrix.
  glm::mat4 CalcProjMat(const CompFrustum& frustum);

  /// Calculate the Transformation(Model) matrix.
  glm::mat4 CalcTransMat(const CompTransform& trans);

}

#endif // CAMERA_UTILS_HPP
