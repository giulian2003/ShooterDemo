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

#include "sys_bullets.hpp"
#include "scene.hpp"
#include "Q3Map.hpp"

using namespace shooter;
using namespace glm;

void SysBullets::Update(float dt, const Q3Map& map, Scene& scene)
{
  // update bullets life time
  int32_t nrValidBullets = scene.nrValidBullets;

  if (!nrValidBullets)
  {
    return;
  }

  for (int32_t i = 0; i < nrValidBullets; i++)
  {
    scene.bullets[i].timeInt -= dt;
  }

  // move expired bullets to the back of scene.bullets
  int32_t index1 = 0;
  int32_t index2 = nrValidBullets - 1;
  while (true) // O(n) complexity
  {
    // find the next valid bullet 
    while ((index1 <= index2) && (scene.bullets[index2].timeInt < FLT_EPSILON))
    {
      nrValidBullets--;
      index2--;
    }

    // find the next expired bullet
    while ((index1 <= index2) && (scene.bullets[index1].timeInt > FLT_EPSILON))
    {
      index1++;
    }

    if (index1 >= index2)
    {
      break;
    }

    std::swap(scene.bullets[index1], scene.bullets[index2]);
  }

  // remove expired bullets
  assert(nrValidBullets >= 0);
  scene.nrValidBullets = nrValidBullets;
}

void SysBullets::FireBullet(const glm::vec3& start, const glm::vec3& end, float timeInt, Scene& scene)
{
  if (scene.nrValidBullets < scene.bullets.size())
  {
    scene.bullets[scene.nrValidBullets++] = CompBullet(start, end, timeInt);
  }
}