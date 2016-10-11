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

#include "controllers.hpp"
#include "scene.hpp"
#include "Q3Map.hpp"
#include "sys_animation.hpp"
#include "resources.hpp"

#include <iostream>

#include <SDL.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/rotate_vector.hpp> 

using namespace shooter;
using namespace glm;
using namespace std;

void PlayerController::HandleEvent(
  const SDL_Event& e,
  Scene& scene)
{
  const float cMovementSensitivity = 6.f; // meters per second
  const float cJumpSensitivity = 5.f; // meters per second
  const float cMouseSensitivity = 0.3f;

  CompState& st = scene.states[EnPlayer];
  CompStatesTimeIntervals& stTimeInt = scene.statesTimeInts[EnPlayer];
  CompTransform& transform = scene.transforms[EnPlayer];
  CompMovable& movable = scene.movables[EnPlayer];

  switch (e.type) {

  case SDL_KEYDOWN:
    if (e.key.repeat == 0)
    {
      switch (e.key.keysym.sym)
      {
      case SDLK_F1:
        scene.debugging = !scene.debugging;
        break;

      case SDLK_F2:
        scene.multithreading = !scene.multithreading;
        break;

      case SDLK_SPACE:
        if ((st.state & EStateOffGround) == 0)
        {
          movable.velocity.y = cJumpSensitivity;
          break;
        }
      }
    }

    // falltrough

  case SDL_KEYUP:
    if (e.key.repeat == 0)
    {
      const float movement = cMovementSensitivity * (e.type == SDL_KEYDOWN ? 1.f : -1.f);

      switch (e.key.keysym.sym) 
      {
      case SDLK_LEFT:
      case SDLK_a:
        movable.velocity.x += -movement;
        break;

      case SDLK_RIGHT:
      case SDLK_d:
        movable.velocity.x += movement;
        break;

      case SDLK_UP:
      case SDLK_w:
        movable.velocity.z += movement;
        break;

      case SDLK_DOWN:
      case SDLK_s:
        movable.velocity.z += -movement;
        break;

      default:
        break;
      }
    }
    break;

  case SDL_MOUSEMOTION:
    transform.front = rotateY(transform.front, radians(-cMouseSensitivity * e.motion.xrel));
    break;

  case SDL_MOUSEBUTTONDOWN:
    if ((e.button.button == 1)
      && (e.button.state == SDL_PRESSED))
    {
      st.state |= EStateShoot; // start shooting
      stTimeInt.timeInts[EStateShootTimeIntIx] = 0.2f;
    }
    break;

  case SDL_MOUSEBUTTONUP:
    if ((e.button.button == 1)
      && (e.button.state == SDL_RELEASED))
    {
      st.state &= ~EStateShoot; // stop shooting
      stTimeInt.timeInts[EStateShootTimeIntIx] = 0.f;
    }
    break;

  default:
    break;
  }
}


void CameraController::HandleEvent(const SDL_Event& e, CompCamera& camera)
{
  switch (e.type) {

  case SDL_MOUSEMOTION:
  {
    const float cMouseSensitivity = 0.3f;
    camera.orientation.y += -cMouseSensitivity * e.motion.xrel;
    camera.orientation.x += -cMouseSensitivity * e.motion.yrel;
    camera.orientation.x = clamp(camera.orientation.x, -30.f, 40.f); // limit pitch
    break;
  }

  case SDL_MOUSEWHEEL:
  {
    const float cWheelSensitivity = .1f;
    fraction += cWheelSensitivity * -e.wheel.y;
    break;
  }

  default:
    break;
  }
}

void CameraController::Update(
  float dt,
  const Q3Map& map,
  const vec3& fixedPosition,
  CompCamera& camera)
{
  // Sync camera.orientation with camera.trans.front
  vec3 front(0.f, 0.f, -1.f);
  front = rotateX(front, radians(camera.orientation.x));
  front = rotateY(front, radians(camera.orientation.y));
  camera.trans.front = normalize(front);

  bool firstPersView(fraction <= FLT_EPSILON);
  fraction = clamp(fraction, 0.f, 1.f);

  if (firstPersView)
  {
    camera.trans.position = fixedPosition + front * 0.6f;
  }
  else
  {
    vec3 start = fixedPosition - front * minDistance;
    vec3 end = start - front * mix(minDistance, maxDistance, fraction);

    TraceData data(start, end, 0.2f);
    map.Trace(data);
    if (!data.mStartsOut)
    {
      camera.trans.position = fixedPosition + front * 0.6f;
      fraction = 0.f;
    }
    else
    {
      if (!data.mCollision)
      {
        camera.trans.position = data.mEnd;
      }
      else
      {
        camera.trans.position = mix(data.mStart, data.mEnd, data.mFraction);
      }
    }
  }
}