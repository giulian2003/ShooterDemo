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

#include "sys_player_shoot.hpp"
#include "sys_attack.hpp"
#include "sys_bullets.hpp"
#include "scene.hpp"
#include "resources.hpp"
#include "intersect_utils.hpp"
#include "constants.hpp" // cMaxShootingPitch

using namespace shooter;
using namespace glm;

void SysPlayerShoot::Update(float dt, const Resources& resources, Scene& scene)
{
  uint32_t state = scene.states[EnPlayer].state;

  if (state & (EStateOffGround|EStateDead))
  {
    // We have no animation for shooting while jumping
    return;
  }

  if (!(state & EStateShoot))
  {
    return;
  }

  if (abs(scene.camera.orientation.x) > cMaxShootingPitch)
  {
    return;
  }

  float animTime = scene.animations[EnPlayer].timeInSeconds;
  if (animTime < FLT_EPSILON)
  {
    // don't shoot when interpolating from the last animation
    return;
  }

  const vec3& vel = scene.movables[EnPlayer].velocity;
  if ((vel.z < -FLT_EPSILON) && (abs(vel.z) > abs(vel.x)))
  {
    // We have no animation for shooting while moving backwards
    return;
  }

  float& shootTimeInt = scene.statesTimeInts[EnPlayer].timeInts[EStateShootTimeIntIx];
  if (shootTimeInt > FLT_EPSILON)
  {
    return;
  }

  const Model& model = resources.GetModel(scene.renderables[EnPlayer].modelName);
  vec3 bulletOrigin = SysAttack::WeaponMuzzlePos(scene.weaponBoneIx, model, scene.transforms[EnPlayer], scene.animations[EnPlayer]);
  vec3 bulletDir = scene.camera.trans.front;

  // intersect bullet with all game objects
  int32_t intersectEntity = -1;
  float intersectDistance = 0.f;
  float damageMultiplier = 0.f;
  SysAttack::IntersectRayEntities(
    EnPlayer, bulletOrigin, bulletDir,
    resources,
    scene.renderables.data(),
    scene.transforms.data(),
    scene.bounds.data(),
    scene.animations.data(),
    scene.damagebles.data(),
    scene.transforms.size(),
    intersectEntity, intersectDistance, damageMultiplier);

  if (intersectEntity >= 0)
  {
    SysAttack::DamageEntity(EnPlayer, intersectEntity, damageMultiplier, scene);
  }

  SysBullets::FireBullet(bulletOrigin, bulletOrigin + bulletDir * intersectDistance, 0.05f, scene);

  shootTimeInt = .1f; // repeat shooting
}