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

#include "sys_states_time_ints.hpp"
#include "components.hpp"

using namespace shooter;

void SysStatesTimeInts::Update(float dt, CompStatesTimeIntervals* statesTimeInts, uint32_t enCount)
{
  // Update the Time Intervals state params 
  for (uint32_t i = 0; i < enCount; i++)
  {
    for (uint32_t j = 0; j < EStateTimeIntMax; j++)
    {
      float& timeInt = statesTimeInts[i].timeInts[j];
      if (timeInt > dt)
      { // only update the states with a positive time interval
        timeInt -= dt;
      }
      else
      {
        timeInt = 0.f;
      }
    }
  }
}
