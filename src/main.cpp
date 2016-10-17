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

#include "resources.hpp"
#include "camera_utils.hpp"
#include "scene.hpp"
#include "constants.hpp"
#include "Q3Loader.h"
#include "Q3Map.hpp"
#include "nav_mesh.hpp"
#include "sys_renderer.hpp"
#include "sys_animation.hpp"
#include "sys_attack.hpp"
#include "sys_bullets.hpp"
#include "sys_evade.hpp"
#include "sys_patrol.hpp"
#include "sys_physics.hpp"
#include "sys_player_shoot.hpp"
#include "sys_revive.hpp"
#include "sys_states_time_ints.hpp"

#include <iostream>
#include <string>

#include <GL\glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <ctpl/ctpl_stl.h>
#include <nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;

using namespace shooter;

static void GLAPIENTRY openglCallbackFunction(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam)
{
  std::cout << "---------------------opengl-callback-start------------" << std::endl;
  std::cout << "message: " << message << std::endl;
  std::cout << "type: ";
  switch (type) {
  case GL_DEBUG_TYPE_ERROR:
    std::cout << "ERROR";
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    std::cout << "DEPRECATED_BEHAVIOR";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    std::cout << "UNDEFINED_BEHAVIOR";
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    std::cout << "PORTABILITY";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    std::cout << "PERFORMANCE";
    break;
  case GL_DEBUG_TYPE_OTHER:
    std::cout << "OTHER";
    break;
  }
  std::cout << std::endl;

  std::cout << "id: " << id << std::endl;
  std::cout << "severity: ";
  switch (severity) {
  case GL_DEBUG_SEVERITY_LOW:
    std::cout << "LOW";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    std::cout << "MEDIUM";
    break;
  case GL_DEBUG_SEVERITY_HIGH:
    std::cout << "HIGH";
    break;
  }
  std::cout << std::endl;
  std::cout << "---------------------opengl-callback-end--------------" << std::endl;
}

void RenderDebugInfo(NVGcontext* vg, int fps, const Scene& scene)
{
  nvgBeginFrame(vg, SCREEN_WIDTH, SCREEN_HEIGHT, 1.f);
  nvgFontSize(vg, 20.0f);
  nvgFontFace(vg, "sans");
  nvgFillColor(vg, nvgRGBA(0, 255, 0, 128));
  nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

  // draw crosshair
  const unsigned crosshairHalfSize = 10; // pixels
  nvgBeginPath(vg);
  nvgMoveTo(vg, SCREEN_WIDTH / 2 - crosshairHalfSize, SCREEN_HEIGHT / 2);
  nvgLineTo(vg, SCREEN_WIDTH / 2 + crosshairHalfSize, SCREEN_HEIGHT / 2);
  nvgMoveTo(vg, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - crosshairHalfSize);
  nvgLineTo(vg, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + crosshairHalfSize);
  nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 255));
  nvgStroke(vg);

  char buf[100] = { 0 };

  snprintf(buf, sizeof(buf), "FPS: %d", fps);
  nvgText(vg, 10, 10, buf, NULL);

  snprintf(buf, sizeof(buf), "Debugging (F1): %s", (scene.debugging ? "ON" : "OFF"));
  nvgText(vg, 10, 30, buf, NULL);

  snprintf(buf, sizeof(buf), "Multithreading (F2): %s", (scene.multithreading ? "ON" : "OFF"));
  nvgText(vg, 10, 50, buf, NULL);

  nvgEndFrame(vg);
}

bool InitScreen(SDL_Window*& screen, SDL_GLContext& context, NVGcontext*& vg)
{
  if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
  {
    std::cout << "SDL_Init failed: " << SDL_GetError() << std::endl;
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG); // debug context

  screen = SDL_CreateWindow("ShooterDemo", SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

  if (screen == nullptr)
  {
    std::cout << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
    return false;
  }

  context = SDL_GL_CreateContext(screen);

  glewExperimental = GL_TRUE;
  GLenum glew_status = glewInit();
  if (glew_status != GLEW_OK)
  {
    std::cout << glewGetErrorString(glew_status) << std::endl;
    return false;
  }

  GLint varMajor = 2, verMinor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &varMajor);
  glGetIntegerv(GL_MINOR_VERSION, &verMinor);
  if ((varMajor < 4) || (verMinor < 5))
  {
    std::cout << "Wrong OpenGL version: " << varMajor << "." << verMinor << std::endl;
    return false;
  }

  // Enable the debug callback
  if (false/*glDebugMessageCallback*/)
  {
    GLuint unusedIds = 0;
    std::cout << "Register OpenGL debug callback " << std::endl;
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(openglCallbackFunction, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
  }

  vg = nvgCreateGL3(NVG_ANTIALIAS);
  if (vg == nullptr)
  {
    std::cout << "nvgCreateGL3 failed." << std::endl;
    return false;
  }

  if (nvgCreateFont(vg, "sans", "res/fonts/Roboto-Bold.ttf") == -1)
  {
    std::cout << "nvgCreateFont failed. " << std::endl;
    return false;
  }

  // VSync off
  SDL_GL_SetSwapInterval(0);

  // Capture the mouse movement outside the window
  SDL_SetRelativeMouseMode(SDL_TRUE);

  return true;
}

inline CompDamagebleBone MakeDamagebleBone(
  const NamesAndIdsMap& bonesMap, 
  const std::vector<int16_t>& bonesHierarchy, 
  const std::string& boneName, 
  float radius, float health)
{
  uint32_t ix = bonesMap.at(boneName);
  return CompDamagebleBone(ix, bonesHierarchy[ix], radius, health);
}

bool InitScene(Resources& resources, Scene& scene)
{
  scene.mapPath = "maps/jof3dm2.zip";
  const std::string modelName("models/ArmyPilot/ArmyPilot.x");

  if (!resources.LoadPrograms("shaders")) { return false; }
  if (!resources.LoadModel(modelName)) { return false; }
  if (!resources.LoadMap(scene.mapPath)) { return false; }
  if (!resources.LoadSkyBox("skybox/DarkStormy/DarkStormy")) { return false; }

  const Model& playerModel = resources.GetModel(modelName);
  float playerSize = cPlayerHeight; // meters
  float playerScale = playerSize * playerModel.normScale;

  CompDamagebleSkeleton damSkeleton;
  // ToDo: read this from a config file
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "Spine2", 0.18f, 1.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "Spine3", 0.18f, 1.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "Neck1", 0.18f, 5.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "NeckHead", 0.08f, 5.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "Rbrow", 0.07f, 5.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "Lbrow", 0.07f, 5.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "RArmUpper2", 0.08f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "RArmForearm1", 0.07f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "RArmForearm2", 0.06f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "RArmHand", 0.05f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "LArmUpper2", 0.08f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "LArmForearm1", 0.07f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "LArmForearm2", 0.06f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "LArmHand", 0.05f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "LLegCalf", 0.08f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "LLegAnkle", 0.06f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "LLegToe1", 0.05f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "RLegCalf", 0.08f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "RLegAnkle", 0.06f, 2.f));
  damSkeleton.skeleton.push_back(MakeDamagebleBone(playerModel.nodesMap, playerModel.nodesParents, "RLegToe1", 0.05f, 2.f));

  for (unsigned i = 0; i < EnNpcMax; i++)
  {
    scene.renderables[i].modelName = modelName;
    scene.animations[i].Set("Idle");
    scene.transforms[i].scale = playerScale;
    scene.bounds[i].minBound = playerModel.minBound * playerScale;
    scene.bounds[i].maxBound = playerModel.maxBound * playerScale;
    glm::vec3 radii = max(abs(playerModel.minBound), abs(playerModel.maxBound)) * playerScale;
    scene.bounds[i].radiusXZ = std::max(radii.x, radii.z);
    scene.states[i].state = EStateDead;
    scene.damagebles[i] = damSkeleton;
  }

  scene.weaponBoneIx = playerModel.nodesMap.at("M4MB");

  return true;
}

int main(int argc, char* args[])
{
  //freopen("log.txt", "w", stdout);

  SDL_Window* screen = nullptr;
  SDL_GLContext context = nullptr;
  NVGcontext* vg = nullptr;
  if (!InitScreen(screen, context, vg)) { return 0; }

  Scene scene;
  Resources resources("res/");
  if (!InitScene(resources, scene)) { return 0; }

  SysRenderer renderer;

  // Init the thread pool
  unsigned nrThreads = std::max(2u, std::thread::hardware_concurrency() - 1);
  ctpl::thread_pool tp(nrThreads);

  // Event handler
  SDL_Event e;

  // Random seed
  srand(SDL_GetTicks());

  uint32_t lastTime = SDL_GetTicks() - static_cast<uint32_t>(cFixedTimeStep * 1000.f);
  float leftOverTime = 0.f;
  int fps = 0, framesCnt = 0;
  uint32_t fpsTime = lastTime + 1000;

  bool quit = false;
  while (!quit)
  {
    // Handle events on queue
    while (SDL_PollEvent(&e) != 0)
    {
      // User requests quit
      if (e.type == SDL_QUIT)  
      {
        quit = true;
      } 
      else 
      {
        scene.cameraController.HandleEvent(e, scene.camera);
        scene.playerController.HandleEvent(e, scene);
      }
    }

    uint32_t currentTime = SDL_GetTicks();

    // Update FPS counter
    if (currentTime > fpsTime)
    {
      fpsTime += 1000;
      fps = framesCnt;
      framesCnt = 0;
    }

    framesCnt++;

    float elapsedTime = (currentTime - lastTime) * 0.001f; // seconds
    lastTime = currentTime;

    // Add time that couldn't be used last frame
    elapsedTime += leftOverTime;

    // Divide it up in chunks of cTimeStep ms
    int steps = (int)floor(elapsedTime / cFixedTimeStep);

    // Store time we couldn't use for the next frame.
    leftOverTime = elapsedTime - steps * cFixedTimeStep;

    for (int i = 0; i < steps; i++) 
    { 
      // The Simulation uses a constant time step
      SysRevive::Update(cFixedTimeStep, resources.GetNavMesh(), scene);
      SysStatesTimeInts::Update(cFixedTimeStep, &scene.statesTimeInts[0], EnNpcMax);
      SysPlayerShoot::Update(cFixedTimeStep, resources, scene);
      SysPatrol::Update(cFixedTimeStep, resources.GetNavMesh(), scene, tp);
      SysAttack::Update(cFixedTimeStep, resources, scene);
      SysEvade::Update(cFixedTimeStep, resources.GetNavMesh(), scene, tp);
      SysPhysics::Update(cFixedTimeStep, resources.GetMap(), resources.GetNavMesh(), scene, tp);
      SysAnimation::Update(cFixedTimeStep, resources, scene, tp);
      SysBullets::Update(cFixedTimeStep, resources.GetMap(), scene);
    }

    glm::vec3 camLookAt = scene.transforms[EnPlayer].position + glm::vec3(0.f, cPlayerHeight, 0.f);

    scene.cameraController.Update(elapsedTime, resources.GetMap(), camLookAt, scene.camera);

    // Clear screen 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Render scene
    renderer.Render(resources, scene);

    // Render debug info
    RenderDebugInfo(vg, fps, scene);

    // Update screen
    SDL_GL_SwapWindow(screen);
  }

  nvgDeleteGL3(vg);
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(screen);
  SDL_Quit();

  return 0;
}