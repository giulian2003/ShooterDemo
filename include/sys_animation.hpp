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

#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <string>

#include <glm/vec3.hpp>

#include <ctpl/ctpl_stl.h>

namespace shooter {

  class Resources;
  struct CompCamera;
  struct Model;
  struct Scene;
  struct CompState;
  struct CompRenderable;
  struct CompMovable;
  struct CompAnimation;

  /// Animation System (see https://en.wikipedia.org/wiki/Entity_component_system). 
  /// Animates the 3D models, interpolating smoothly when changing between different animations. 
  class SysAnimation
  {
  public:
    
    /// Update function. Called from the game loop at cFixedTimeStep time intervals.
    static void Update(float timeInSeconds, const Resources& resources, Scene& scene, ctpl::thread_pool& tp);

    /// Determine the type of the animation based on different parameters.
    static std::string GetAnimation(const glm::vec3& vel, uint32_t state, float absCamPitch, bool isNPC);

  private:

    /// Thread safe update function.
    static void UpdateEntity(
      int /*ThreadId*/,
      float timeInSeconds,
      uint32_t entity,
      const Resources& resources,
      const CompState& state,
      const CompRenderable& renderable,
      const CompMovable& movable,
      const CompCamera& camera,
      CompAnimation& anim);
  };

}
#endif //ANIMATION_HPP