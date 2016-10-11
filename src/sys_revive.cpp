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

#include "sys_revive.hpp"
#include "scene.hpp"
#include "nav_mesh.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp> // distance2

using namespace shooter;
using namespace glm;

void SysRevive::Update(float dt, const NavMesh& navMesh, Scene& scene)
{
  const std::vector<float>& revivePositions = navMesh.GetIntersectionPositions();

  uint32_t objCount = scene.transforms.size();
  // for each dead game entity
  for (uint32_t i = 0; i < objCount; i++)
  {
    uint32_t& state = scene.states[i].state;
    if (!(state & EStateDead))
    {
      continue;
    }

    float& deadTimeInt = scene.statesTimeInts[i].timeInts[EStateDeadTimeIntIx];
    if (deadTimeInt > FLT_EPSILON)
    {
      continue;
    }

    // choose a random revive position
    bool validRevivePos = true;
    int posIx = rand() % (revivePositions.size() / 3);
    vec3 revivePos = make_vec3(&revivePositions[posIx * 3]);

    // check if the revivePos is valid
    for (uint32_t j = 0; j < objCount; j++)
    {
      if ((scene.states[j].state & EStateDead) && (j != i))
      {
        continue;
      }

      if (distance2(revivePos, scene.transforms[j].position) < 25.f)
      {
        validRevivePos = false;
        break;
      }
    }

    if (validRevivePos)
    {
      state = EStateOffGround;
      if (i != EnPlayer)
      {
        state |= EStatePatrol;
      }
      scene.transforms[i].position = revivePos;
      scene.health[i].health = 100.f;
    }
    else
    {
      deadTimeInt = 0.2f; // recheck later
    }
  }
}