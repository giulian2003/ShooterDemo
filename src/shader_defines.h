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

/// Vertex shader input parameters locations
#define VERT_POSITION_LOC 0
#define VERT_NORMAL_LOC 1
#define VERT_COLOR_LOC 2
#define VERT_DIFFUSE_TEX_COORD_LOC 3
#define VERT_LIGHTMAP_TEX_COORD_LOC 4
#define VERT_BONE_IDS_LOC 5
#define VERT_BONE_WEIGHTS_LOC 6

//// Fragment shader output parameters locations
#define FRAG_COLOR_LOC 0

/// Uniform Bindings Points
#define MATRICES_BINDING 1
#define MATERIAL_BINDING 2
#define LIGHT_BINDING 3
#define BONES_BINDING 4

/// Uniform locations
#define LIGHT_POSITION_LOC 0
#define CAMERA_POSITION_LOC 1

#define BILLBOARD_ORIGIN_LOC 2
#define BILLBOARD_ROTATION_AXIS 3
#define BILLBOARD_IN_WORLD_SPACE 4
#define BILLBOARD_WIDTH 5
#define BILLBOARD_HEIGHT 6
#define CAMERA_POS_LOC 7
#define GLOBAL_TIME_LOC 8

/// Texture Units
#define DIFFUSE_TEX_UNIT 0
#define LIGHTMAP_TEX_UNIT 1

const int MAX_BONES = 100;
