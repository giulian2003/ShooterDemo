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

#ifndef CONTROLLERS_HPP
#define CONTROLLERS_HPP

#include <string>

#include <glm/vec3.hpp>

union SDL_Event;

namespace shooter {

  class Q3Map;
  struct Scene;
  struct CompCamera;

  /// Handles the player movement
  class PlayerController {
  public:
    static void HandleEvent(const SDL_Event& e, Scene& scene);
  };

  /// Handles the camera movement
  /// The camera moves along the front vector between min and max distances from the fixedPosition
  class CameraController {
  public:
    CameraController(float pMinDistance, float pMaxDistance)
      : minDistance(pMinDistance), maxDistance(pMaxDistance), fraction(1.f)
    {}

    void HandleEvent(const SDL_Event& e, CompCamera& camera);

    void Update(
      float dt,
      const Q3Map& map,
      const glm::vec3& fixedPosition,
      CompCamera& camera);

    const float minDistance, maxDistance;
    float fraction; ///< describes the camera position = fixedPosition + front * lerp(minDistance, maxDistance, fraction)
  };
}

#endif //CONTROLLERS_HPP