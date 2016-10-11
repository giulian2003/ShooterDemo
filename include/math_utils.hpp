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

#ifndef MATH_UTILS_HPP
#define MATH_UTILS_HPP

#include <stdlib.h>

#include <glm/glm.hpp>

#include "constants.hpp"

namespace shooter {

  const float cSinRotationStep = glm::sin(cFixedTimeStep * cRotationSpeed);
  const float cCosRotationStep = glm::cos(cFixedTimeStep * cRotationSpeed);

  const glm::tmat3x3<float> cRotationStepMat(
    cCosRotationStep, 0.f, -cSinRotationStep,
    0.f, 1.f, 0.f,
    cSinRotationStep, 0.f, cCosRotationStep);

  const glm::tmat3x3<float> cMinusRotationStepMat(
    cCosRotationStep, 0.f, cSinRotationStep,
    0.f, 1.f, 0.f,
    -cSinRotationStep, 0.f, cCosRotationStep);

  /// Rotate v1 towards v2 with a constant cRotationSpeed
  /// @return true if the resulting v1 is the same as v2, false otherwise
  inline bool RotateYFixedStep(glm::vec3& v1, const glm::vec3& v2)
  {
    float cosAlfa = dot(v1, v2);
    if (cosAlfa > cCosRotationStep - FLT_EPSILON)
    {
      v1 = v2;
      return true;
    }

    assert((v1.y == 0.f) && (v2.y == 0.f));
    float sinAlfa = cross(v1, v2).y;
    v1 = (sinAlfa > 0.f ? cRotationStepMat : cMinusRotationStepMat) * v1;

    return false;
  }

  /// Normalize a vec3 avoiding division by 0
  inline glm::vec3 SafeNormalize(const glm::vec3& v, const glm::vec3& v0 = glm::vec3(1.f, 0.f, 0.f))
  {
    float len = length(v);
    return (len < FLT_EPSILON ? v0 : v / len);
  }

  /// Return a random number between min and max
  inline float RandRange(float min, float max)
  {
    return min + rand() / (float)RAND_MAX * (max - min);
  }

  /// Return + or - 1 randomly
  inline float RandSgn()
  {
    return ((rand() & 1) << 1) - 1.f;
  }
}
#endif // MATH_UTILS_HPP