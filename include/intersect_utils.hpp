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

#ifndef INTERSECT_UTILS_HPP
#define INTERSECT_UTILS_HPP

#include <glm/vec3.hpp>

namespace shooter {

  /// Ray/Cylinder intersection check.
  /// Inspired from http://www.gamedev.net/topic/467789-raycylinder-intersection/
  bool intersectRayCylinder(
    const glm::vec3& rayO, ///< Ray Origin
    const glm::vec3& rayV, ///< Ray direction
    const glm::vec3& cylA, ///< Cylinder extremity 1
    const glm::vec3& cylB, ///< Cylinder extremity 2
    float cylR, ///< Cylinder radius
    float& outRayDist ///< Intersection distance from the ray origin. Intersection point is rayO + rayV * outRayDist 
  );
}

#endif // INTERSECT_UTILS_HPP