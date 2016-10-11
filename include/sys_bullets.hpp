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

#ifndef SYS_BULLETS_HPP
#define SYS_BULLETS_HPP

#include <glm/vec3.hpp>

namespace shooter
{
  class Q3Map;
  struct Scene;

  /// Bullets System (see https://en.wikipedia.org/wiki/Entity_component_system).
  /// Updates the life time of all bullets and removes the expired ones
  class SysBullets
  {
  public:

    /// Update function. Called from the game loop at cFixedTimeStep time intervals.
    static void Update(float dt, const Q3Map& map, Scene& scene);

    /// Adds a bullet to scene.bullets
    static void FireBullet(
      const glm::vec3& start, ///< Bullet's start location
      const glm::vec3& end, ///< Bullet's end location
      float timeInt, ///< Bullet's lifetime
      Scene& scene);
  };
}

#endif // SYS_BULLETS_HPP