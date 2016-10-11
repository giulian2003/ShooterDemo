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

#include "sys_evade.hpp"
#include "sys_attack.hpp" // for KillEntity()
#include "scene.hpp"
#include "nav_mesh.hpp"
#include "constants.hpp"
#include "math_utils.hpp"

#include <glm/gtc/type_ptr.hpp>

using namespace shooter;
using namespace glm;

bool SysEvade::GetSidewaysWalkableDir(const NavMesh& navMesh, const vec3& pos, const vec3& front, float radius, float step, vec3 &outNormDir)
{
  vec3 bestPt;
  float minAbsCosAlfa = FLT_MAX;
  for (float offset = -radius; offset < radius; offset += step)
  {
    vec3 ptOffset[4] = {
      vec3(offset, 0.f, -radius), vec3(radius, 0.f, offset),
      vec3(-offset, 0.f, radius), vec3(-radius, 0.f, -offset)
    };

    for (int i = 0; i < 4; i++)
    {
      // Check positions along the sides of a square centered in pos
      vec3 pt = pos + ptOffset[i];
      float absCosAlfa = abs(dot(normalize(ptOffset[i]), front));

      // Find the walkable position closer to the sideways direction.
      // Checking positions radius away from the NPC's position,
      // helps the NPC escape small concave regions of the Navmesh border.
      bool walkable = false;
      float h = 0.f, floorDist = FLT_MAX, borderDist = 0.f;
      if ((absCosAlfa < minAbsCosAlfa)
        && navMesh.GetFloorInfo(value_ptr(pt), 1.f, h, floorDist, &walkable, &borderDist)
        && walkable
        && (borderDist > 0.07f))
      {
        minAbsCosAlfa = absCosAlfa;
        bestPt = pt;
      }
    }
  }

  if (minAbsCosAlfa > 1.f)
  {
    return false;
  }

  outNormDir = normalize(bestPt - pos);

  return true;
}

void SysEvade::Evade(float targetDist, float borderDist, vec3& vel, float& timeInt)
{
  if (targetDist < cEvadeBackAwayDist
    // allow the NPC to exit a concave portion of the NavMesh border 
    // when trying to get away from another NPC
    && borderDist > 0.07f)
  {
    // move away from the target
    vel.z = -RandRange(cMinEvadeVelZ, cMaxEvadeVelZ);
  }
  else if (targetDist > cEvadeApproachDist
    // allow the NPC to exit a concave portion of the NavMesh border 
    // when trying to get closer to another NPC
    && borderDist > 0.07f)
  {
    // move towards the target
    vel.z = RandRange(cMinEvadeVelZ, cMaxEvadeVelZ);
  }
  else if (timeInt < FLT_EPSILON)
  {
    vel.z = RandSgn() * RandRange(cMinEvadeVelZ, cMaxEvadeVelZ);
  }

  if (timeInt < FLT_EPSILON)
  {
    vel.x = RandSgn() * RandRange(cMinEvadeVelX, cMaxEvadeVelX);
    timeInt = RandRange(.5f, 1.5f); //seconds
  }
}

void SysEvade::UpdateEntity(
  int /*ThreadId*/,
  const NavMesh& navMesh,
  const vec3& targetPos,
  const CompTransform& trans, 
  const CompStatesTargets& stTargets, 
  CompState& st, 
  CompStatesTimeIntervals& stTimeInt,
  CompMovable& movable,
  CompHealth& health,
  CompScore& score)
{
  const uint32_t& state = st.state;
  float& timeInt = stTimeInt.timeInts[EStateEvadeTimeIntIx];
  vec3& vel = movable.velocity;
  const vec3& pos = trans.position;

  vec3 vTarget = pos - targetPos;
  vTarget.y = 0.f;
  float targetDist = length(vTarget);

  bool walkable = false;
  float h = 0.f, floorDist = FLT_MAX, borderDist = 0.f;
  bool haveFloorInfo = navMesh.GetFloorInfo(value_ptr(pos), 1.f, h, floorDist, &walkable, &borderDist);
  bool nearBorder = haveFloorInfo && (!walkable || borderDist < 0.001f);

  Evade(targetDist, borderDist, vel, timeInt);

  // If run into the edge of the walkable area
  if (nearBorder)
  {
    const vec3& front = trans.front;
    assert(front.y == 0.f);

    vec3 side = cross(front, cWorldUp);

    vec3 newDir;
    if (!SysEvade::GetSidewaysWalkableDir(navMesh, pos, front, 0.8f, 0.2f, newDir))
    {
      // This happens when a NPC jumps on top of another NPC and 
      // slides down far outside the walkable area
      SysAttack::KillEntity(health, st, stTimeInt, score);
      return;
    }

    // the Bot will remain oriented towards the target and  
    // change the velocity so it starts moving towards newDir
    vec3 newVel = RandRange(cMinEvadeVelX, cMaxEvadeVelX) * newDir;
    vel.z = dot(front, newVel);
    vel.x = dot(side, newVel);
    timeInt = RandRange(.5f, 1.5f); //seconds
  }
}

void SysEvade::Update(float dt, const NavMesh& navMesh, Scene& scene, ctpl::thread_pool& tp)
{
  uint32_t nrEntities = scene.transforms.size();

  std::vector<std::future <void> > results;
  results.reserve(nrEntities);

  for (uint32_t i = EnNpcMin; i < nrEntities; i++)
  {
    const uint32_t& state = scene.states[i].state;

    if (state & (EStateOffGround|EStateDead))
    {
      // can't evade while jumping
      continue;
    }

    vec3& vel = scene.movables[i].velocity;

    if (!(state & EStateEvade))
    {
      vel.x = 0;
      continue;
    }

    const int32_t& targetIx = scene.statesTargets[i].targets[EStateAttackTargetIx];

    // the positions don't get modified by UpdateEntity, so it's safe to access without syncronization
    const vec3& targetPos = scene.transforms[targetIx].position;

    if (scene.multithreading)
    {
      // Add the job to the Thread Pool
      results.push_back(
        tp.push(
          UpdateEntity,
          std::cref(navMesh),
          std::cref(targetPos),
          std::cref(scene.transforms[i]),
          std::cref(scene.statesTargets[i]),
          std::ref(scene.states[i]),
          std::ref(scene.statesTimeInts[i]),
          std::ref(scene.movables[i]),
          std::ref(scene.health[i]),
          std::ref(scene.scores[i])));
    }
    else
    {
      UpdateEntity(i,
        navMesh,
        targetPos,
        scene.transforms[i],
        scene.statesTargets[i],
        scene.states[i],
        scene.statesTimeInts[i],
        scene.movables[i],
        scene.health[i],
        scene.scores[i]);
    }
  }

  if (scene.multithreading)
  {
    // Wait for the jobs to finish
    for (auto& res : results)
    {
      res.wait();
    }
  }

}