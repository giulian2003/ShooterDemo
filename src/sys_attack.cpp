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

#include "sys_attack.hpp"
#include "sys_bullets.hpp"
#include "scene.hpp"
#include "constants.hpp"
#include "camera_utils.hpp"
#include "intersect_utils.hpp"
#include "math_utils.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp> // length2
#include <glm/gtx/intersect.hpp> // intersectRaySphere

using namespace shooter;
using namespace glm;

float SysAttack::CalcPriority(float dist, float cosAlfa1, float cosAlfa2)
{
  assert(dist >= 0.f && dist <= 1.f);
  assert(cosAlfa1 >= 0.f && cosAlfa1 <= 1.f);
  assert(cosAlfa2 >= 0.f && cosAlfa2 <= 1.f);

  return 10.f * (1.f - dist) + 3.f * cosAlfa1 + 6.f * cosAlfa2;
}

int32_t SysAttack::FindTarget(int32_t srcEntity, const Q3Map& map, const CompTransform* transforms, const CompState* states, uint32_t nrEntities)
{
  typedef std::pair<int32_t, float> TIndexAndPriority;
  std::vector<std::pair<int32_t, float> > priorities;
  priorities.reserve(nrEntities - 1);

  vec3 srcPos = transforms[srcEntity].position;

  for (uint32_t i = 0; i < nrEntities; i++)
  {
    if (i == srcEntity) 
    { 
      continue; 
    }

    if (states[i].state & EStateDead)
    {
      continue;
    }

    const vec3& dstPos = transforms[i].position;
    vec3 dirXZ = dstPos - srcPos;
    float hSq = dirXZ.y * dirXZ.y;
    dirXZ.y = 0.f;

    float distSq = length2(dirXZ);

    if ((distSq > FLT_EPSILON)
      && (distSq < cAttackDistanceSq)
      && ((hSq < 4.f) || (hSq / distSq < .1f)))
    {
      float dist = sqrt(distSq);
      dirXZ /= dist; // normalize

      // Clamp to positive range because we only care about the targets in front of the current obj
      float cosAlfa1 = clamp(dot(dirXZ, transforms[srcEntity].front), 0.f, 1.f);
      float cosAlfa2 = clamp(dot(-dirXZ, transforms[i].front), 0.f, 1.f);

      float priority = CalcPriority(dist / cAttackDistance, cosAlfa1, cosAlfa2);
      
      priorities.push_back(TIndexAndPriority(i, priority));
    }
  }

  // Optimization: Minimize the number of ray casts by ordering the priorities in deacreasing order 
  // and returning the index of the first target found visible
  std::stable_sort(begin(priorities), end(priorities), [](const auto& it1, const auto& it2) { return it1.second > it2.second; });
  for (const auto& pr : priorities)
  {
    vec3 dH(0.f, 1.3f, 0.f);
    TraceData data(srcPos + dH, transforms[pr.first].position + dH, .5f);
    if (!map.Trace(data))
    {
      return pr.first;
    }
  }

  return -1;
}

vec3 SysAttack::WeaponMuzzlePos(uint32_t weaponBoneIx, const Model& model, const CompTransform& trans, const CompAnimation& anim)
{
  return vec3(CalcTransMat(trans) * anim.globalTrans[weaponBoneIx] * model.invBonesOffsets[weaponBoneIx] * vec4(-1.f, -67.f, -11.f, 1.f));
}

void SysAttack::KillEntity(
  CompHealth& outHealth,
  CompState& outState,
  CompStatesTimeIntervals& outTimeInt,
  CompScore &inoutScore)
{
  outHealth.health = 0.f;
  outState.state = EStateDead;
  outTimeInt.timeInts[EStateDeadTimeIntIx] = .5f;
  inoutScore.deaths++;
}

void SysAttack::DamageEntity(int32_t enAttacker, int32_t enVictim, float damageMultiplyer, Scene& scene)
{
  float& health = scene.health[enVictim].health;
  health -= cWeaponDamage * damageMultiplyer;
  if (/*enVictim != 0 && */health < FLT_EPSILON)
  {
    scene.scores[enAttacker].kills++;
    KillEntity(scene.health[enVictim], scene.states[enVictim], scene.statesTimeInts[enVictim], scene.scores[enVictim]);
  }
}

void SysAttack::IntersectRayEntities(
  int32_t srcEntity,
  const vec3& rayOrigin,
  const vec3& rayDir,
  const Resources& resources,
  const CompRenderable* renderables,
  const CompTransform* transforms,
  const CompBounds* bounds,
  const CompAnimation* anim,
  const CompDamagebleSkeleton* damSkeleton,
  uint32_t nrEntities,
  int32_t& outIntersectEntity,
  float& outIntersectDistance,
  float& outDamageMultiplier)
{
  // intersect ray with the Map
  float rayMaxDist = 1000.f;
  TraceData data(rayOrigin, rayOrigin + rayDir * rayMaxDist);
  resources.GetMap().Trace(data);
  //assert(data.mCollision);

  // Make sure the ray doesn't pass trough the walls
  rayMaxDist *= data.mFraction;

  outIntersectDistance = rayMaxDist;
  outIntersectEntity = -1;
  outDamageMultiplier = 0.f;

  typedef std::pair<uint32_t, float> TIndexAndDistance;
  std::vector<TIndexAndDistance> sphereIntersectedObjs;
  sphereIntersectedObjs.reserve(nrEntities - 1);

  // do a rejection test using the entities bounding spheres
  for (uint32_t i = 0; i < nrEntities; i++)
  {
    if (i == srcEntity)
    {
      // same Entity as the shooter
      continue;
    }

    const CompTransform& t = transforms[i];
    const CompBounds& b = bounds[i];
    vec3 extends = max(-b.minBound, b.maxBound);

    float sphereIntersectDist = FLT_MAX;
    if (intersectRaySphere(rayOrigin, rayDir, t.position, length2(extends), sphereIntersectDist)
      && (sphereIntersectDist <= rayMaxDist))
    {
      sphereIntersectedObjs.push_back(std::make_pair(i, sphereIntersectDist));
    }
  }

  // Optimization: Minimize the number of intersections with the damageble skeleton by
  // ordering the entities in increasing distance order and stopping at the first intersected entity
  std::sort(sphereIntersectedObjs.begin(), sphereIntersectedObjs.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
  for (const auto& item : sphereIntersectedObjs)
  {
    uint32_t en = item.first;
    mat4 modelMat = CalcTransMat(transforms[en]);
    const Model& model = resources.GetModel(renderables[en].modelName);

    float damageMul = 0.f;
    float minIntersectDist = rayMaxDist;
    // Intersect ray with all the cylinders in the damageble skeleton
    for (const CompDamagebleBone& damBone : damSkeleton[en].skeleton)
    {
      vec3 cylA(modelMat * anim[en].globalTrans[damBone.boneIx1] * model.invBonesOffsets[damBone.boneIx1][3]);
      vec3 cylB(modelMat * anim[en].globalTrans[damBone.boneIx2] * model.invBonesOffsets[damBone.boneIx2][3]);

      float cylinderIntersectDist = rayMaxDist;
      if (shooter::intersectRayCylinder(rayOrigin, rayDir, cylA, cylB, damBone.radius, cylinderIntersectDist)
        && (cylinderIntersectDist < minIntersectDist))
      {
        minIntersectDist = cylinderIntersectDist;
        damageMul = damBone.damageMul;
      }
    }

    if (minIntersectDist < rayMaxDist)
    {
      outIntersectDistance = minIntersectDist;
      outIntersectEntity = en;
      outDamageMultiplier = damageMul;
      return;
    }
  }
}

void SysAttack::CheckTarget(int32_t enNewTarget, CompState& st, CompStatesTargets& stTargets, CompStatesTimeIntervals& stTimeInts)
{
  uint32_t& state = st.state;
  TStateTargets& stateTargets = stTargets.targets;
  TStateTimeInts& stateTimeInts = stTimeInts.timeInts;

  int32_t& enOldTarget = stateTargets[EStateAttackTargetIx];
  const float& huntTimeInt = stateTimeInts[EStateHuntTimeIntIx];

  if ((enNewTarget != enOldTarget) && (huntTimeInt < FLT_EPSILON))
  {
    if (enNewTarget >= 0)
    {
      // start attacking the new target
      state |= EStateAttack | EStateEvade;
      enOldTarget = enNewTarget;
      stateTargets[EStateEvadeTargetIx] = enNewTarget;
      stateTimeInts[EStateEvadeTimeIntIx] = 0.f;

      // stop patrolling or hunting
      state &= ~(EStatePatrol | EStateHunt);
      stateTargets[EStateHuntTargetIx] = -1;
      stateTimeInts[EStateHuntTimeIntIx] = 0.f;
    }
    else
    {
      // start hunting the old target
      state |= EStateHunt;
      stateTargets[EStateHuntTargetIx] = enOldTarget;
      stateTimeInts[EStateHuntTimeIntIx] = .5f; // seconds

      // stop attacking the current target
      state &= ~(EStateAttack | EStateShoot | EStateEvade);
      enOldTarget = -1;
      stateTargets[EStateEvadeTargetIx] = -1;
      stateTimeInts[EStateEvadeTimeIntIx] = 0.f;
    }
  }
}

bool SysAttack::AimAtTarget(uint32_t attackerState, CompTransform& attackerTrans, const CompBounds& attackerBounds, const CompTransform& targetTrans)
{
  // look towards the target
  vec3 dir = targetTrans.position - attackerTrans.position;
  if (attackerState & EStateShoot)
  {
    // ToDo: Fix this
    // when shooting, account for the weapon's position which is to the right from the model's center
    dir -= normalize(cross(attackerTrans.front, cWorldUp)) * attackerBounds.maxBound.x * 0.5f;
  }

  dir.y = 0.f;
  dir = normalize(dir);

  // rotate towards the target
  return RotateYFixedStep(attackerTrans.front, dir);
}

bool SysAttack::TryShootingAtTarget(uint32_t& state, float& shootTimeInt, bool lookingAtTarget)
{
  bool shooting = ((state & EStateShoot) != 0);
  bool canFireBullet = (shooting && (shootTimeInt < FLT_EPSILON));

  if (lookingAtTarget)
  {
    if (!shooting)
    {
      // start shooting
      state |= EStateShoot;
    }

    if (shootTimeInt < FLT_EPSILON)
    {
      // fire bullet
      shootTimeInt = cShootingRepeatTime;
      return true;
    }
  }
  else
  {
    // stop shooting
    state &= ~EStateShoot;
    shootTimeInt = 0.f;
  }

  return false;
}

vec3 SysAttack::BulletDirection(vec3 bulletOrigin, const CompTransform& attackerTrans, const CompTransform& targetTrans)
{
  float x = RandRange(-1.f, 1.f);
  float y = RandRange(-1.f, 1.f);
  vec3 side = cross(attackerTrans.front, cWorldUp);
  vec3 bulletPos = targetTrans.position + cWorldUp * (1 + y * 0.5f) + side * (x * 0.5f);
  return normalize(bulletPos - bulletOrigin);
}

void SysAttack::Update(float dt, const Resources& resources, Scene& scene)
{
  const Q3Map& map = resources.GetMap();
  uint32_t objCount = scene.transforms.size();

  bool fireBullet = false;
  for (uint32_t i = EnNpcMin; i < objCount; i++)
  {
    int32_t newTarget = FindTarget(i, map, scene.transforms.data(), scene.states.data(), objCount);
    CheckTarget(newTarget, scene.states[i], scene.statesTargets[i], scene.statesTimeInts[i]);

    uint32_t& state = scene.states[i].state;
    
    if (state & (EStateOffGround | EStateDead))
    {
      // can't attack while jumping
      continue;
    }

    if (!(state & (EStateAttack)))
    {
      continue;
    }

    int32_t& target = scene.statesTargets[i].targets[EStateAttackTargetIx];
    assert((state & EStateAttack) && (target >= 0));

    CompTransform& trans = scene.transforms[i];
    const CompTransform& targetTrans = scene.transforms[target];
    float& shootTimeInt = scene.statesTimeInts[i].timeInts[EStateShootTimeIntIx];

    bool lookingAtTarget = AimAtTarget(state, trans, scene.bounds[i], targetTrans);

    if (TryShootingAtTarget(state, shootTimeInt, lookingAtTarget))
    {
      const Model& model = resources.GetModel(scene.renderables[i].modelName);
      vec3 bulletOrigin = WeaponMuzzlePos(scene.weaponBoneIx, model, trans, scene.animations[i]);
      vec3 bulletDir = BulletDirection(bulletOrigin, trans, targetTrans);

      // intersect bullet with the Map and all Entities
      int32_t intersectEntity = -1;
      float intersectDistance = 0.f;
      float damageMultiplier = 0.f;
      IntersectRayEntities(
        i, bulletOrigin, bulletDir,
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
        DamageEntity(i, intersectEntity, damageMultiplier, scene);
      }

      SysBullets::FireBullet(bulletOrigin, bulletOrigin + bulletDir * intersectDistance, 0.05f, scene);

      // Optimization: The ray casting and intersection with the entities when firing a bullet 
      // are quite expensive, so only fire one bullet per frame
      break;
    }
  }
}