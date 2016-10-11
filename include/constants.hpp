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

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

#include <glm/vec3.hpp>

namespace shooter
{
  const float cFixedTimeStep = 0.016f;
  const float cPi = 3.14159265359f;

  const float cPlayerHeight = 1.8f; // meters

  const float cRotationSpeed = 1.5f * cPi; // radians per second

  /// Entities evasion speeds on the X and Z axii
  const float cMinEvadeVelZ = 1.5f;
  const float cMaxEvadeVelZ = 2.f;
  const float cMinEvadeVelX = 2.f;
  const float cMaxEvadeVelX = 3.f;
  const float cEvadeApproachDist = 10.f;
  const float cEvadeBackAwayDist = 5.f;

  const float cMinPatrolVelZ = 3.f;
  const float cMaxPatrolVelZ = 4.f;

  const float cAttackDistance = 20.f;
  const float cAttackDistanceSq = cAttackDistance * cAttackDistance;
  const float cWeaponDamage = 5.f;

  const float cMaxShootingPitch = 20.f; // degrees
  const float cShootingRepeatTime = .2f; // degrees

  const float cMaxWalkingSpeed = 3.5f;

  const glm::vec3 cWorldUp(0.f, 1.f, 0.f);
  const glm::vec3 cGravity(0.f, -10.f, 0.f);
  const glm::vec3 cJumpVel(0.f, 4.2f, 3.f);
}

#endif // CONSTANTS_HPP
