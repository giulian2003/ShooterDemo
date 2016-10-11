#version 430

layout (std140, binding = MATRICES_BINDING) uniform Matrices {
  mat4 u_projMat;
  mat4 u_viewMat;
  mat4 u_modelMat;
  mat4 u_normalMat; // we only consider the 3x3 matrix
};

// attributes
layout(location = VERT_POSITION_LOC) in vec3 i_position;
layout(location = VERT_DIFFUSE_TEX_COORD_LOC) in vec2 i_texCoordDiffuse;

// data for fragment shader
out vec2 o_texCoordDiffuse;

void main()
{
  // texture coordinates to fragment shader
  o_texCoordDiffuse = i_texCoordDiffuse;

  // screen space coordinates of the vertex
  gl_Position = u_projMat * u_viewMat * u_modelMat * vec4(i_position, 1.f);
}