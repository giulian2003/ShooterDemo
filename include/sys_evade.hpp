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

#ifndef SYS_EVADE_HPP
#define SYS_EVADE_HPP

#include <glm/vec3.hpp>

#include <ctpl/ctpl_stl.h>

namespace shooter
{
  class NavMesh;
  struct CompTransform;
  struct CompMovable;
  struct Scene;
  struct CompState;
  struct CompStatesTargets;
  struct CompStatesTimeIntervals;
  struct CompHealth;
  struct CompScore;

  /// Evade System (see https://en.wikipedia.org/wiki/Entity_component_system).
  /// Contains the logic related to evasion when a NPC attacks another NPC or the Player
  class SysEvade
  {
  public:

    /// Update function. Called from the game loop at cFixedTimeStep time intervals.
    static void Update(float dt, const NavMesh& navMesh, Scene& scene, ctpl::thread_pool& tp);

  private:

    /// Thread safe update function.
    static void UpdateEntity(
      int /*ThreadId*/,
      const NavMesh& navMesh,
      const glm::vec3& targetPos,
      const CompTransform& trans,
      const CompStatesTargets& stTargets,
      CompState& st,
      CompStatesTimeIntervals& stTimeInt,
      CompMovable& movable,
      CompHealth& health,
      CompScore& score);

    /// Return a valid direction an entity can move towards when it reached the NavMesh border
    static bool GetSidewaysWalkableDir(
      const NavMesh& navMesh, 
      const glm::vec3& pos, ///< Entity position
      const glm::vec3& front, ///< Entity orientation
      float radius, ///< Halfside of a square centered in pos that will be checked for valid directions
      float step, ///< Value used for optimization, usually the size of the NavMesh cell
      glm::vec3 &outNormDir);

    /// Change Entity's velocity and timeInt to make it harder to shoot by an enemy Entity
    static void Evade(float targetDist, float borderDist, glm::vec3& vel, float& timeInt);
  };

}

#endif // SYS_EVADE_HPP