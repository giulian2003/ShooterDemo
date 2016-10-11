#version 450

// attributes
layout(location = VERT_POSITION_LOC) in vec3 i_position;            // xyz - position
layout(location = VERT_NORMAL_LOC) in vec3 i_normal;              // xyz - normal
layout(location = VERT_DIFFUSE_TEX_COORD_LOC) in vec2 i_texCoordDiffuse;    // xy - texture coords
layout(location = VERT_LIGHTMAP_TEX_COORD_LOC) in vec2 i_texCoordLightmap;    // xy - texture coords

// data for fragment shader
out vec3 tes_o_normal;
out vec2 tes_o_texCoordDiffuse;
out vec2 tes_o_texCoordLightmap;

void main()
{
  // normal in view space
  tes_o_normal = i_normal;

  // texture coordinates to fragment shader
  tes_o_texCoordDiffuse = i_texCoordDiffuse;
  tes_o_texCoordLightmap = i_texCoordLightmap;

  // screen space coordinates of the vertex
  gl_Position = vec4(i_position, 1.f);
}
