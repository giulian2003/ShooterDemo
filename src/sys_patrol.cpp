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

#include "sys_patrol.hpp"
#include "scene.hpp"
#include "constants.hpp"
#include "math_utils.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp> // for distance2()

using namespace shooter;
using namespace glm;

void SysPatrol::UpdateEntity(
  int /*ThreadId*/,
  const NavMesh& navMesh,
  vec3 huntTargetPos,
  const CompState& st, 
  const vec3& pos,
  vec3& front,
  CompStatesTargets& stTargets, 
  CompNavMeshPath& patrol, 
  CompNavMeshPos& navMeshPos, 
  CompMovable& movable
)
{
  if (!patrol.nrPathPolys)
  {
    // Calculate a new path to a random position on the NavMesh
    vec3 pathEnd;
    if (st.state & EStateHunt)
    {
      pathEnd = huntTargetPos;
    }
    else
    {
      const std::vector<float>& patrolPos = navMesh.GetIntersectionPositions();
      int posIx = rand() % (patrolPos.size() / 3);
      pathEnd = make_vec3(&patrolPos[posIx * 3]);
    }

    navMesh.FindPath(glm::value_ptr(pos), glm::value_ptr(pathEnd),
      glm::value_ptr(patrol.pathStartPos), glm::value_ptr(patrol.pathEndPos), patrol.pathPolys, patrol.nrPathPolys);

    movable.velocity = vec3(0.f, 0.f, RandRange(cMinPatrolVelZ, cMaxPatrolVelZ));
  }

  // Update the steering position
  glm::vec3 steerPos;
  bool offMeshConn = false, endOfPath = false;
  navMesh.GetSteerPosOnPath(glm::value_ptr(pos), glm::value_ptr(patrol.pathEndPos), navMeshPos.visitedPolys, navMeshPos.nrPolys,
    patrol.pathPolys, patrol.nrPathPolys, 0.1f, glm::value_ptr(steerPos), offMeshConn, endOfPath);

  vec3 steerDir = steerPos - pos;
  steerDir.y = 0.f; // should always be on the XZ plane
  steerDir = SafeNormalize(steerDir, front);

  // Update orientation
  if (offMeshConn)
  {
    front = steerDir;
  }
  else if (length2(front - steerDir) > FLT_EPSILON)
  {
    RotateYFixedStep(front, steerDir);
  }

  // Update velocity
  if (offMeshConn)
  {
    movable.velocity = cJumpVel;
  }
  else if (endOfPath)
  {
    movable.velocity = vec3();
  }
}

void SysPatrol::Update(float dt, const NavMesh& navMesh, Scene& scene, ctpl::thread_pool& tp)
{
  uint32_t nrEntities = scene.transforms.size();

  std::vector<std::future <void> > results;
  results.reserve(nrEntities);

  for (uint32_t i = 0; i < nrEntities; i++)
  {
    // Do all the early outs in the current thread to avoid the overhead 
    // when calling UpdateEntity in a new thread
    uint32_t state = scene.states[i].state;

    if (state & (EStateOffGround | EStateDead)) 
    { 
      continue; 
    }

    if (!(state & (EStatePatrol | EStateHunt)))
    {
      scene.navMeshPath[i].nrPathPolys = 0;
      continue;
    }

    vec3 huntTargetPos;
    if (state & EStateHunt)
    {
      int32_t targetIx = scene.statesTargets[i].targets[EStateHuntTargetIx];
      assert(targetIx >= 0);

      uint32_t huntTargetState = scene.states[targetIx].state;
      if (scene.states[targetIx].state & EStateOffGround) 
      { 
        continue; 
      }

      // the positions don't get modified by UpdateEntity, so it's safe to access without syncronization
      huntTargetPos = scene.transforms[targetIx].position;
    }

    // Unfortunately, Detour is not thread safe (https://groups.google.com/forum/#!topic/recastnavigation/r7gL4F552m4)
    if (false/*scene.multithreading*/)
    {
      results.push_back(
        tp.push(
          UpdateEntity,
          std::cref(navMesh),
          std::cref(huntTargetPos),
          std::cref(scene.states[i]),
          std::cref(scene.transforms[i].position),
          std::ref(scene.transforms[i].front),
          std::ref(scene.statesTargets[i]),
          std::ref(scene.navMeshPath[i]),
          std::ref(scene.navMeshPos[i]),
          std::ref(scene.movables[i])));
    }
    else
    {
      UpdateEntity(
        0,
        navMesh,
        huntTargetPos,
        scene.states[i],
        scene.transforms[i].position,
        scene.transforms[i].front,
        scene.statesTargets[i],
        scene.navMeshPath[i],
        scene.navMeshPos[i],
        scene.movables[i]);
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