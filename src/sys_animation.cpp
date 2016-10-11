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

#include "sys_animation.hpp"
#include "resources.hpp"
#include "scene.hpp"
#include "constants.hpp" // cMaxShootingPitch

using namespace glm;
using namespace shooter;

std::string SysAnimation::GetAnimation(const glm::vec3& vel, uint32_t state, float absCamPitch, bool isNPC)
{
  std::string animName;

  float ax = abs(vel.x), az = abs(vel.z);
  float speed = sqrt(ax*ax + az*az);

  bool moveForward = (vel.z > FLT_EPSILON) && (az > ax);
  bool moveBackwards = (vel.z < -FLT_EPSILON) && (az > ax);
  bool moveLeft = (vel.x < -FLT_EPSILON) && (ax >= az);
  bool moveRight = (vel.x > FLT_EPSILON) && (ax >= az);
  bool run = (speed > cMaxWalkingSpeed);
  bool shoot = ((state & EStateShoot) != 0) && ((absCamPitch < cMaxShootingPitch) || isNPC);

  // The order of the ifs matters !
  if (state & EStateDead)
  {
    animName = "Standing_2";
  }
  else if (state & EStateOffGround)
  {
    animName = "Jump";
  }
  else if (moveForward)
  {
    if (run)
    {
      animName = shoot ? "Run_Firing" : "Run_Forwards";
    }
    else
    {
      animName = shoot ? "Walk_Firing" : "Walk";
    }
  }
  else if (moveBackwards)
  {
    if (run)
    {
      animName = "Run_backwards";
    }
    else
    {
      animName = "Walk_Backwards";
    }
  }
  else if (moveLeft)
  {
    animName = shoot ? "Left_Fire" : "Strafe_Left";
  }
  else if (moveRight)
  {
    animName = shoot ? "Right_FIre" : "Strafe_Right";
  }
  else
  {
    animName = shoot ? "Idle_Firing" : "Idle";
  }

  return animName;
}

void SysAnimation::UpdateEntity(
  int /*ThreadId*/,
  float timeInSeconds, 
  uint32_t entity,
  const Resources& resources,
  const CompState& state,
  const CompRenderable& renderable,
  const CompMovable& movable,
  const CompCamera& camera,
  CompAnimation& anim)
{
  anim.timeInSeconds += timeInSeconds;

  std::string animName = GetAnimation(movable.velocity, state.state, abs(camera.orientation.x), entity > 0);

  if (anim.name != animName)
  {
    anim.Set(animName, -cAnimationTransitionTime);
  }

  resources.GetSkeletonTransforms(
    resources.GetModel(renderable.modelName),
    anim.name,
    anim.timeInSeconds,
    anim.lastTimeInSeconds,
    anim.lastAnimationFrames,
    anim.globalTrans);
}

void SysAnimation::Update(float timeInSeconds, const Resources& resources, Scene& scene, ctpl::thread_pool& tp)
{
  uint32_t nrEntities = scene.transforms.size();

  if (scene.multithreading)
  {
    // Add the jobs to the Thread Pool
    std::vector<std::future <void> > results(nrEntities);
    for (unsigned i = 0; i < nrEntities; i++)
    {
      results[i] = tp.push(
        UpdateEntity,
        timeInSeconds,
        i,
        std::cref(resources),
        std::cref(scene.states[i]),
        std::cref(scene.renderables[i]),
        std::cref(scene.movables[i]),
        std::cref(scene.camera),
        std::ref(scene.animations[i]));
    }

    // Wait for the jobs to finish
    for (auto& res : results)
    {
      res.wait();
    }
  }
  else
  {
    for (unsigned i = 0; i < nrEntities; i++)
    {
        UpdateEntity(
          0,
          timeInSeconds,
          i,
          resources,
          scene.states[i],
          scene.renderables[i],
          scene.movables[i],
          scene.camera,
          scene.animations[i]);
    }
  }
}
