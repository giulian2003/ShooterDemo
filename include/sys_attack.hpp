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

#ifndef SYS_ATTACK_HPP
#define SYS_ATTACK_HPP

#include <glm/vec3.hpp>

namespace shooter
{
  class Resources;
  class Q3Map;
  struct Scene;
  struct Model;
  struct CompState;
  struct CompStatesTargets;
  struct CompStatesTimeIntervals;
  struct CompRenderable;
  struct CompTransform;
  struct CompBounds;
  struct CompAnimation;
  struct CompDamagebleSkeleton;
  struct CompHealth;
  struct CompScore;

  /// Attack System (see https://en.wikipedia.org/wiki/Entity_component_system).
  /// Contains the logic of how NPCs are attacking other NPCs
  class SysAttack
  {
  public:

    /// Update function. Called from the game loop at cFixedTimeStep time intervals.
    static void Update(float dt, const Resources& resources, Scene& scene);

    /// Damage an entity and kill it if it's life reached 0%.
    static void DamageEntity(
      int32_t enAttacker, ///< Attacker entity
      int32_t enVictim, ///< Victim entity
      float damageMultiplyer, ///< Damage multiplier (based on the bodypart hit)
      Scene& scene);
    
    /// Kill an entity.
    static void KillEntity(
      CompHealth& outHealth,
      CompState& outState,
      CompStatesTimeIntervals& outTimeInt,
      CompScore &inoutScore);

    /// Return the weapon muzzle position, taking in consideration the current animation state.
    static glm::vec3 WeaponMuzzlePos(
      uint32_t weaponBoneIx,
      const Model& model,
      const CompTransform& trans,
      const CompAnimation& anim);

    /// Intersect Ray with the map and all the Entities
    static void IntersectRayEntities(
      int32_t srcEntity, ///< Entity from which the Ray originated
      const glm::vec3& rayOrigin, ///< Ray origin
      const glm::vec3& rayDir, ///< Ray direction
      const Resources& resources,
      const CompRenderable* renderables,
      const CompTransform* transforms,
      const CompBounds* bounds,
      const CompAnimation* anim,
      const CompDamagebleSkeleton* damSkeleton,
      uint32_t nrEntities,
      int32_t& outIntersectEntity, ///< [out] Intersected entity
      float& outIntersectDistance, ///< [out] Distance to the intersected entity
      float& outDamageMultiplier ///< [out] Damage multiplyier (based on the bodypart hit by the ray)
    );

  private:
    
    /// Calculates an Attack priority based on the distance between 2 entities and their orientations.
    static float CalcPriority(
      float dist, ///< Distance between 2 entities
      float cosAlfa1, ///< Cos of angle between the 2 entities vector and the 1st entity front vector
      float cosAlfa2 ///< Cos of angle between the 2 entities vector and the 2nd entity front vector
    );
    
    /// Return the visible target entity with the highest priority or -1 if none was found.
    static int32_t FindTarget(int32_t srcEntity, const Q3Map& map, const CompTransform* transforms, const CompState* states, uint32_t nrEntities);
    
    /// Starts hunting or attacking if we have a new target.
    static void CheckTarget(int32_t enNewTarget, CompState& states, CompStatesTargets& statesTargets, CompStatesTimeIntervals& statesTimeInts);
    
    /// Rotate smoothly to face the target entity.
    static bool AimAtTarget(uint32_t attackerState, CompTransform& attackerTrans, const CompBounds& attackerBounds, const CompTransform& targetTrans);
    
    /// Try to fire repeatedly at the target entity.
    static bool TryShootingAtTarget(uint32_t& state, float& shootTimeInt, bool lookingAtTarget);
    
    /// Calculate the bullet's direction based on different parameters.
    static glm::vec3 BulletDirection(glm::vec3 bulletOrigin, const CompTransform& attackerTrans, const CompTransform& targetTrans);
  };
}

#endif // SYS_ATTACK_HPP