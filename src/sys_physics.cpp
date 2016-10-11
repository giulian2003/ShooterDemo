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

#include <sys_physics.hpp>
#include <Q3Map.hpp>
#include <nav_mesh.hpp>
#include <scene.hpp>
#include <constants.hpp>
#include <DetourNavMeshQuery.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp> // distance2
#include <glm/gtx/compatibility.hpp> // lerp

using namespace shooter;
using namespace glm;

bool SysPhysics::TryClimbStep(
  const Q3Map& map,
  const vec3& minBounds,
  const vec3& maxBounds,
  vec3& inoutPos, 
  const vec3& dir)
{
  // Try to climb up and down the stairs
  vec3 endPos = inoutPos + dir;
  TraceData dataStep(endPos + vec3(0, 0.5f, 0), endPos, minBounds, maxBounds);
  map.Trace(dataStep);
  if (!dataStep.mStartsOut)
  {
    return false;
  }

  if (dataStep.mCollision)
  {
    inoutPos = mix(dataStep.mStart, dataStep.mEnd, dataStep.mFraction);
  }
  else
  {
    inoutPos = dataStep.mEnd;
  }

  return true;
}

bool SysPhysics::TrySlideWall(
  const Q3Map& map,
  const vec3& minBounds,
  const vec3& maxBounds,
  vec3& inoutPos, 
  vec3& inoutDir)
{
  // Try to slide along the wall
  TraceData data(inoutPos, inoutPos + inoutDir, minBounds, maxBounds);
  if (!map.Trace(data))
  {
    inoutPos = data.mEnd;
    return true;
  }

  inoutPos = mix(data.mStart, data.mEnd, data.mFraction);
  inoutDir = data.mPlaneProj;

  return false;
}

void SysPhysics::FixEntityMapCollision(
  const Q3Map& map, 
  vec3 minBound, 
  vec3 maxBound, 
  vec3& inoutPos, 
  vec3& inoutDir)
{
  if (inoutDir == vec3())
  {
    return;
  }

  vec3 extends = max(-minBound, maxBound);
  float radiusXZ = max(extends.x, extends.z);
  minBound = vec3(-radiusXZ, minBound.y, -radiusXZ);
  maxBound = vec3(radiusXZ, maxBound.y + 0.2f, radiusXZ);

  TraceData data(inoutPos, inoutPos + inoutDir, minBound, maxBound);
  if (!map.Trace(data))
  {
    inoutPos = data.mEnd;
    return;
  }

  // try to slide along max 3 walls
  vec3 newPos = inoutPos, newDir = inoutDir;
  for (int i = 0; i < 3; i++)
  {
    if (TryClimbStep(map, minBound, maxBound, newPos, newDir) ||
        TrySlideWall(map, minBound, maxBound, newPos, newDir))
    {
      inoutPos = newPos;
      inoutDir = newDir;
      break;
    }
  }
}

void SysPhysics::VelocityVerlet(float dt, const glm::vec3& vel, const glm::vec3& acc, glm::vec3& dPos, glm::vec3& dVel)
{
  dPos = dt * (vel + .5f * dt * acc);
  dVel = dt * acc;
}

void SysPhysics::FixEntityCollisions(const CompBounds* bounds, CompTransform* transforms, uint32_t nrEntities)
{
  // Find Collisions

  // This has n square complexity but, because of the low number of entities, it's not a performance problem
  typedef std::pair<unsigned, unsigned> TCollision;
  std::vector<TCollision> collisions;
  for (unsigned i = 0; i < nrEntities - 1; ++i)
  {
    float r1 = bounds[i].radiusXZ;
    vec3 p1 = transforms[i].position;

    for (unsigned j = i + 1; j < nrEntities; ++j)
    {
      float r = r1 + bounds[j].radiusXZ;
      const vec3& p2 = transforms[j].position;

      if (glm::distance2(p1, p2) <= r * r)
      {
        collisions.push_back(std::make_pair(i, j));
      }
    }
  }

  // Fix Collisions
  // No point to use extremely complex collision avoidance algorithms because the game play
  // will make collisions between entities extremely rare (they will shoot each other before colliding)
  for (const auto& col : collisions)
  {
    uint32_t en1(col.first), en2(col.second);
    vec3& p1 = transforms[en1].position;
    vec3& p2 = transforms[en2].position;
    float r1 = bounds[en1].radiusXZ;
    float r2 = bounds[en2].radiusXZ;

    vec3 c = lerp(p1, p2, r1 / (r1 + r2));

    vec3 dir = p2 - c;
    float lenDir = length(dir);
    dir = (lenDir > FLT_EPSILON ? dir / lenDir : vec3(0.f, 1.f, 0.f));

    p2 = c + dir * r2;
    p1 = c - dir * r1;
  }
}

void SysPhysics::UpdateEntity(
  int /*ThreadId*/,
  float dt, 
  const Q3Map& map, 
  const NavMesh& navMesh, 
  const CompBounds& bounds,
  CompState& st,
  CompTransform& trans, 
  CompMovable& movable, 
  CompNavMeshPos& navMeshPos)
{
  uint32_t& state = st.state;
  const vec3& front = trans.front;
  const vec3& pos = trans.position;

  // Calculate the velocity in world space
  mat3 orientation(cross(front, cWorldUp), cWorldUp, front);
  vec3 vel = orientation * movable.velocity;
  vec3 acc = cGravity;

  vec3 polyPos;
  dtPolyRef& poly = navMeshPos.poly;
  static const float polyPickExt[3] = { .01f, 1.f, .01f };
  navMesh.GetNavMeshQuery()->findNearestPoly(value_ptr(pos), polyPickExt, navMesh.GetQueryFilter(), &poly, value_ptr(polyPos));

  // Check if the Entity is on the floor
  bool onFloor = false;
  if (poly)
  {
    onFloor = (pos.y - polyPos.y < 0.1f);
  }
  else
  {
    // There are portions of the map that are outside the NavMesh but still walkable
    float floorH = FLT_MAX, floorDst = 0;
    if (navMesh.GetFloorInfo(value_ptr(pos), 1.f, floorH, floorDst))
    {
      onFloor = (floorDst < 0.2f);
    }
  }

  onFloor = onFloor && (vel.y < FLT_EPSILON);

  if (onFloor)
  {
    state &= ~EStateOffGround;

    acc.y = 0.f;
    vel.y = 0.f;
    movable.velocity.y = 0.f;
  }
  else
  {
    state |= EStateOffGround;
  }

  // Calculate the delta Pos and delta Vel using
  // Velocity Verlet Itegration with constant acceleration
  vec3 dPos, dVel;
  VelocityVerlet(dt, vel, acc, dPos, dVel);

  // Update velocity
  movable.velocity.y += dVel.y;

  // Update position
  if (onFloor && poly)
  {
    // Move along the NavMesh
    dtPolyRef* visitedPolys = navMeshPos.visitedPolys;
    int& nvisited = navMeshPos.nrPolys;

    navMesh.GetNavMeshQuery()->moveAlongSurface(poly, value_ptr(polyPos), value_ptr(polyPos + dPos), navMesh.GetQueryFilter(),
      value_ptr(trans.position), visitedPolys, &nvisited, CompNavMeshPos::cMaxPolys);

    assert(nvisited > 0);
    navMesh.GetNavMeshQuery()->getPolyHeight(visitedPolys[nvisited - 1], value_ptr(trans.position), &trans.position.y);
  }
  else
  {
    // Jump or move outside the NavMesh
    FixEntityMapCollision(map, bounds.minBound + vec3(0.f, 0.5f, 0.f), bounds.maxBound, trans.position, dPos);

    if (vel.y > FLT_EPSILON && dPos.y < FLT_EPSILON)
    {
      // The Entity hit a ceiling while jumping
      movable.velocity.y = 0.f;
    }
  }
}

void SysPhysics::Update(float dt, const Q3Map& map, const NavMesh& navMesh, Scene& scene, ctpl::thread_pool& tp)
{
  const uint32_t nrEntities = scene.transforms.size();

  std::vector<std::future <void> > results;
  results.reserve(nrEntities);

  for (uint32_t i = 0; i < nrEntities; i++)
  {
    const uint32_t& state = scene.states[i].state;

    if (state & EStateDead)
    {
      continue;
    }

    // Unfortunately, Detour is not thread safe (https://groups.google.com/forum/#!topic/recastnavigation/r7gL4F552m4)
    if (false/*scene.multithreading*/)
    {
      results.push_back(
        tp.push(
          UpdateEntity,
          dt,
          std::cref(map),
          std::cref(navMesh),
          std::cref(scene.bounds[i]),
          std::ref(scene.states[i]), 
          std::ref(scene.transforms[i]), 
          std::ref(scene.movables[i]), 
          std::ref(scene.navMeshPos[i])
        ));
    }
    else
    {
      UpdateEntity(0, dt, map, navMesh, scene.bounds[i], scene.states[i], scene.transforms[i], scene.movables[i], scene.navMeshPos[i]);
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

  FixEntityCollisions(scene.bounds.data(), scene.transforms.data(), nrEntities);
}