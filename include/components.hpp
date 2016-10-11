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

#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>

#include <DetourNavMesh.h>

#include "resources.hpp"

// Contains all the Components of the project (see https://en.wikipedia.org/wiki/Entity_component_system)

namespace shooter {

  /// Component containing data necessary for rendering an entity.
  struct CompRenderable
  {
    std::string modelName; ///< Name of the 3D model
  };

  /// Component containing data necessary for scaling, rotating and translating an entity.
  struct CompTransform
  {
    CompTransform() 
      : scale(1.f)
      , front(0.f, 0.f, -1.f) 
    {}

    glm::vec3 position; ///< Position of the entity
    glm::vec3 front; ///< Orientation of the entity as a 3D vector
    float scale; ///< Scale of the entity 
  };

  /// Component containing data necessary for the 3D Camera frustum.
  struct CompFrustum 
  {
    CompFrustum()
      : fov(50.f)
      , aspect_ratio(1.3f)
      , near(.01f)
      , far(1000.f)
    {}

    float fov; ///< Vertical Field of View in degrees
    float aspect_ratio; ///< Aspect ratio
    float near; ///< Distance to the near plane
    float far; ///< Distance to the far plane
  };

  /// Component containing data necessary for the 3D Camera.
  struct CompCamera 
  {
    CompFrustum frustum; ///< Frustum data
    CompTransform trans; ///< Transformation data

    /// Each x,y,z component of the vector represents the rotation
    /// around the respective axis in degrees. Needs to stay syncronized
    /// with trans.front
    glm::vec3 orientation;///< Orientation of the entity.
  };

  /// Component containing data data necessary for positioning an entity on the NavMesh.
  struct CompNavMeshPos
  {
    static const uint32_t cMaxPolys = 16;

    CompNavMeshPos() 
      : poly(0)
      , nrPolys(0) 
    { std::fill_n(visitedPolys, cMaxPolys, 0); }

    dtPolyRef poly;
    dtPolyRef visitedPolys[cMaxPolys];
    int nrPolys;
  };

  /// Component containing bounds of an entity.
  struct CompBounds
  {
    glm::vec3 minBound; ///< Minimum bound of an entity
    glm::vec3 maxBound; ///< Maximum bound of an entity
    float radiusXZ; ///< Bounding radius of an entity on the horizontal plane
  };

  /// Component containing data data necessary for moving an entity.
  struct CompMovable
  {
    glm::vec3 velocity; ///< Velocity of an entity in model space
  };

  /// The transition time between 2 different animations in seconds
  const float cAnimationTransitionTime = .2f;

  /// Component containing data data necessary for animating an entity.
  struct CompAnimation
  {
    CompAnimation() 
      : timeInSeconds(0.f)
      , lastTimeInSeconds(0.f)
    {}

    void Set(const std::string& pName, 
      float pTimeInSeconds = 0.f, 
      float pLastTimeInSeconds = -cAnimationTransitionTime)
    {
      name = pName;
      timeInSeconds = pTimeInSeconds;
      lastTimeInSeconds = pLastTimeInSeconds;
    }

    std::string name; ///< Name of the animation
    float timeInSeconds; ///< Time of the animation, in seconds
    std::vector<glm::mat4> globalTrans; ///< Global transformation matrices of all the animation nodes

    /// Needed when transitioning between 2 animation types
    float lastTimeInSeconds; ///< Time of previous frame's animation
    std::vector<AnimationFrame> lastAnimationFrames; ///< Data describing the previous animation frames
  };

  /// Component containing data data necessary for describing a path between 2 points on the NavMesh.
  struct CompNavMeshPath
  {
    static const int MAX_POLYS = 256;

    CompNavMeshPath()
      : nrPathPolys(0)
    {}

    glm::vec3 pathStartPos; ///< Start position on the NavMesh
    glm::vec3 pathEndPos; ///< End position on the NavMesh
    dtPolyRef pathPolys[MAX_POLYS]; ///< Path's polygon Ids
    int nrPathPolys; ///< Number of polygons in the path
  };

  /// Enum of different states bit masks.
  enum StateBitMask
  {
    EStateDead = 1 << 0,
    EStateOffGround = 1 << 1,
    EStatePatrol = 1 << 2,
    EStateAttack = 1 << 3,
    EStateShoot = 1 << 4,
    EStateEvade = 1 << 5,
    EStateHunt = 1 << 6,
  };

  /// Component containing data data describing the state of an entity.
  struct CompState
  {
    CompState() : state(0) {}

    uint32_t state; ///< Bit mask of states
  };

  typedef std::vector<int32_t> TStateTargets;
  
  enum StatesTargetIndex
  {
    EStateAttackTargetIx = 0,
    EStateHuntTargetIx,
    EStateEvadeTargetIx,
    EStateTargetMax
  };

  /// Component containing the Target Entity for all the states that need this type of parameter.
  /// For example the Target Entity of the Hunt state can be found in targets[EStateHuntTargetIx]
  struct CompStatesTargets
  {
    CompStatesTargets() : targets(EStateTargetMax, -1) {}

    TStateTargets targets;
  };

  typedef std::vector<float> TStateTimeInts;

  enum StatesTimeIntervalIndex
  {
    EStateDeadTimeIntIx = 0,
    EStateShootTimeIntIx,
    EStateHuntTimeIntIx,
    EStateEvadeTimeIntIx,
    EStateTimeIntMax
  };

  /// Component containing the Time Interval for all the states that need this type of parameter.
  /// For example the Time Interval of the Hunt state can be found in timeInts[EStateHuntTimeIntIx]
  struct CompStatesTimeIntervals
  {
    CompStatesTimeIntervals() : timeInts(EStateTimeIntMax, 0.f) {}

    TStateTimeInts timeInts;
  };

  /// Component containing data necessary for rendering a bullet.
  struct CompBullet
  {
    CompBullet() : timeInt(0.f) {}
    CompBullet(const glm::vec3& start, const glm::vec3& end, float timeInt) 
      : timeInt(timeInt), startPos(start), endPos(end) 
    {}

    float timeInt; ///< Bullet's duration
    glm::vec3 startPos; ///< Bullet's start position
    glm::vec3 endPos; ///< Bullet's end position
  };

  /// Component containing data necessary for damaging an area of an entity.
  /// This area has the shape of a Cylinder with a bone as it's main axis.

  struct CompDamagebleBone
  {
    CompDamagebleBone(
      uint16_t pBoneIx1,
      uint16_t pBoneIx2,
      float pRadius,
      float pHealth)
      : boneIx1(pBoneIx1)
      , boneIx2(pBoneIx2)
      , radius(pRadius)
      , damageMul(pHealth)
    {}

    uint16_t boneIx1; ///< Bone index
    uint16_t boneIx2; ///< Parent bone index
    float radius; ///< Cylinder radius
    float damageMul; ///< Damage multiplier
  };


  /// Component containing data necessary for damaging an entity.
  struct CompDamagebleSkeleton
  {
    std::vector<CompDamagebleBone> skeleton;
  };

  /// Component containing the health of an entity.
  struct CompHealth
  {
    CompHealth() : health(0.f) {}
    CompHealth(float pHealth) : health(pHealth) {}

    float health; ///< Health in percentage [0.f, 100.f]
  };

  /// Component containing the score of an entity.
  struct CompScore
  {
    CompScore() : deaths(0), kills(0) {}

    uint32_t deaths; ///< Number of times an entity was killed
    uint32_t kills; ///< Number of times this entity killed other entities
  };
}

#endif //COMPONENTS_HPP