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

#include "intersect_utils.hpp"

#include <glm/glm.hpp>

using namespace glm;

namespace shooter {

  // inspired from http://www.gamedev.net/topic/467789-raycylinder-intersection/
  //
  // Ray : P(t) = O + V * t
  // Cylinder [A, B, r].
  // Point P on infinite cylinder if ((P - A) x (B - A))^2 = r^2 * (B - A)^2
  // expand : ((O - A) x (B - A) + t * (V x (B - A)))^2 = r^2 * (B - A)^2
  // equation in the form (X + t * Y)^2 = d
  // where : 
  //  X = (O - A) x (B - A)
  //  Y = V x (B - A)
  //  d = r^2 * (B - A)^2
  // expand the equation :
  // t^2 * (Y . Y) + t * (2 * (X . Y)) + (X . X) - d = 0
  // => second order equation in the form : a*t^2 + b*t + c = 0 where
  // a = (Y . Y)
  // b = 2 * (X . Y)
  // c = (X . X) - d
  bool intersectRayCylinder(
    const vec3& rayO, const vec3& rayV,
    const vec3& cylA, const vec3& cylB, float cylR,
    float& outRayDist)
  {
    vec3 AB = cylB - cylA;
    vec3 AO = rayO - cylA;
    vec3 X = cross(AO, AB);
    vec3 Y = cross(rayV, AB);
    float ab2 = dot(AB, AB);
    float d = cylR * cylR * ab2;
    float a = dot(Y, Y);
    float b = 2.f * dot(X, Y);
    float c = dot(X, X) - d;

    float delta = b * b - 4.f * a * c;
    if ((a < FLT_EPSILON) || (delta < -FLT_EPSILON))
    {
      return false;
    }

    float sqrtDelta = sqrt(delta);
    float t1 = 0.5f * (-b - sqrtDelta) / a;
    float t2 = 0.5f * (-b + sqrtDelta) / a;

    if (t1 > t2)
    {
      float t = t1;
      t1 = t2;
      t2 = t;
    }

    float t[] = { t1, t2 };
    for (int i = 0; i < 2; i++)
    {
      vec3 P = rayO + rayV * t[i];
      float h = dot(P - cylA, AB) / ab2;
      if ((h >= 0.f) && (h <= 1.f))
      {
        outRayDist = t[i];
        return true;
      }
    }

    return false;
  }
}