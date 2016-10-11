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

#ifndef SCENE_HPP
#define SCENE_HPP

#include <string>

#include <glm/vec3.hpp>

#include "components.hpp"
#include "controllers.hpp"

namespace shooter {

  /// Entities (see https://en.wikipedia.org/wiki/Entity_component_system). 
  enum Entities
  {
    EnPlayer = 0,
    EnNpcMin = 1,
    EnNpcMax = 8,
  };

  /// Structure containing the current game state
  struct Scene
  {
    Scene() 
      : renderables(EnNpcMax)
      , animations(EnNpcMax)
      , transforms(EnNpcMax)
      , bounds(EnNpcMax)
      , movables(EnNpcMax)
      , navMeshPath(EnNpcMax)
      , navMeshPos(EnNpcMax)
      , states(EnNpcMax)
      , statesTargets(EnNpcMax)
      , statesTimeInts(EnNpcMax)
      , damagebles(EnNpcMax)
      , health(EnNpcMax, CompHealth(100.f))
      , scores(EnNpcMax)
      , bullets(100)
      , nrValidBullets(0u)
      , cameraController(0.1f, 1.f)
      , debugging(false)
      , multithreading(true)
    {}

    /// Preallocated arrays containing components, one for each entity. 
    
    std::vector<CompRenderable> renderables;
    std::vector<CompAnimation> animations;
    std::vector<CompTransform> transforms;
    std::vector<CompBounds> bounds;
    std::vector<CompMovable> movables;
    std::vector<CompNavMeshPath> navMeshPath;
    std::vector<CompNavMeshPos> navMeshPos;
    std::vector<CompState> states;
    std::vector<CompStatesTargets> statesTargets;
    std::vector<CompStatesTimeIntervals> statesTimeInts;
    std::vector<CompDamagebleSkeleton> damagebles;
    std::vector<CompHealth> health;
    std::vector<CompScore> scores;

    std::vector<CompBullet> bullets; ///< Preallocated bullets
    unsigned nrValidBullets; ///< Number of valid bullets

    PlayerController playerController; ///< Controls the player movement

    CompCamera camera; ///< Camera component
    CameraController cameraController; ///< Controls the camera movement

    uint32_t weaponBoneIx; ///< Bone index of the model's weapon

    std::string mapPath; ///< Path to the map zip file

    bool debugging; ///< Toggle debugging information
    bool multithreading; ///< Toggle multithreading
  };

}

#endif //SCENE_HPP