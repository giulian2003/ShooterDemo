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

#ifndef SYS_PATROL_HPP
#define SYS_PATROL_HPP

#include <glm/vec3.hpp>

#include <ctpl/ctpl_stl.h>

namespace shooter
{
  class NavMesh;
  struct Scene;
  struct CompState;
  struct CompStatesTargets;
  struct CompStatesTimeIntervals;
  struct CompNavMeshPath;
  struct CompTransform;
  struct CompNavMeshPos;
  struct CompMovable;
  struct CompAnimation;

  /// Patrol System (see https://en.wikipedia.org/wiki/Entity_component_system).
  /// Contains the logic related to NPCs randomly patroling the map or hunting another NPC
  class SysPatrol
  {
  public:

    /// Update function. Called from the game loop at cFixedTimeStep time intervals.
    static void Update(float dt, const NavMesh& navMesh, Scene& scene, ctpl::thread_pool& tp);

  private:

    /// Thread safe update function.
    static void UpdateEntity(
      int /*ThreadId*/,
      const NavMesh& navMesh,
      glm::vec3 huntTargetPos,
      const CompState& st,
      const glm::vec3& transPos,
      glm::vec3& transFront,
      CompStatesTargets& stTargets,
      CompNavMeshPath& patrol,
      CompNavMeshPos& navMeshPos,
      CompMovable& movable);
  };
}

#endif // SYS_PATROL_HPP