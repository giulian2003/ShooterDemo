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

#ifndef SYS_PHYSICS_HPP
#define SYS_PHYSICS_HPP

#include <glm/vec3.hpp>

#include <ctpl/ctpl_stl.h>

namespace shooter
{
  class Q3Map;
  class NavMesh;
  struct Scene;
  struct CompState;
  struct CompTransform;
  struct CompMovable;
  struct CompBounds;
  struct CompNavMeshPos;

  /// Physics System (see https://en.wikipedia.org/wiki/Entity_component_system).
  /// Does the physics simulation and solves the collision between 
  /// the different entities and the map.
  class SysPhysics
  {
  public:

    /// Update function. Called from the game loop at cFixedTimeStep time intervals.
    static void Update(float dt, const Q3Map& map, const NavMesh& navMesh, Scene& scene, ctpl::thread_pool& tp);

  private:

    /// Thread safe update function.
    static void UpdateEntity(
      int /*ThreadId*/,
      float dt,
      const Q3Map& map,
      const NavMesh& navMesh,
      const CompBounds& bounds,
      CompState& st,
      CompTransform& trans,
      CompMovable& movable,
      CompNavMeshPos& navMeshPos);

    /// Physics simulation using Velocity Verlet Integration with constant acceleration.
    static void VelocityVerlet(
      float dt, 
      const glm::vec3& vel, 
      const glm::vec3& acc, 
      glm::vec3& dPos, 
      glm::vec3& dVel
    );

    /// Climb up stairs when outside the NavMesh walkable area.
    static bool TryClimbStep(
      const Q3Map& map,
      const glm::vec3& minBounds, ///< Min Entity bound
      const glm::vec3& maxBounds, ///< Max Entity bound
      glm::vec3& inoutPos, ///< Entity's position
      const glm::vec3& dir ///< Entity's movement vector (delta position)
    );

    /// Slide along walls when outside the NavMesh walkable area.
    static bool TrySlideWall(
      const Q3Map& map,
      const glm::vec3& minBounds, ///< Min Entity bound
      const glm::vec3& maxBounds, ///< Max Entity bound
      glm::vec3& inoutPos, ///< Entity's position
      glm::vec3& inoutDir ///< Entity's movement vector (delta position)
    );

    /// Collide Entity with the Map
    static void FixEntityMapCollision(
      const Q3Map& map, 
      glm::vec3 minBound, ///< Min Entity bound
      glm::vec3 maxBound, ///< Max Entity bound
      glm::vec3& inoutPos, ///< Entity's position
      glm::vec3& inoutDir ///< Entity's movement vector (delta position)
    );

    /// Collide Entities with each other
    static void FixEntityCollisions(
      const CompBounds* bounds,
      CompTransform* transforms,
      uint32_t nrEntities);
  };
}
#endif // SYS_PHYSICS_HPP