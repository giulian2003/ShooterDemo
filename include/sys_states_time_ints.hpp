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

#ifndef SYS_STATES_TIME_INTS_HPP
#define SYS_STATES_TIME_INTS_HPP

#include <cstdint>

namespace shooter
{
  struct CompStatesTimeIntervals;

  /// States Time Interval System (see https://en.wikipedia.org/wiki/Entity_component_system).
  /// Update all the time intervals states parameters.
  class SysStatesTimeInts
  {
  public:

    /// Update function. Called from the game loop at cFixedTimeStep time intervals.
    static void Update(float dt, CompStatesTimeIntervals* statesTimeInts, uint32_t objCount);
  };

}
#endif // SYS_STATES_TIME_INTS_HPP