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

#ifndef SYS_REVIVE_HPP
#define SYS_REVIVE_HPP

namespace shooter
{
  class NavMesh;
  struct Scene;

  /// Entity Revival System (see https://en.wikipedia.org/wiki/Entity_component_system).
  /// Revives dead entities.
  class SysRevive
  {
  public:

    /// Update function. Called from the game loop at cFixedTimeStep time intervals. 
    static void Update(float dt, const NavMesh& navMesh, Scene& scene);
  };
}

#endif // SYS_REVIVE_HPP