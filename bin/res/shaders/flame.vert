#version 430

layout (std140, binding = MATRICES_BINDING) uniform Matrices {
  mat4 u_projMat;
  mat4 u_viewMat;
  mat4 u_modelMat;
  mat4 u_normalMat; // we only consider the 3x3 matrix
};

layout(location = BILLBOARD_ORIGIN_LOC) uniform vec3 u_BBOrigin;
layout(location = BILLBOARD_ROTATION_AXIS) uniform vec3 u_BBRotAxis;
layout(location = BILLBOARD_IN_WORLD_SPACE) uniform bool u_BBInWorldSpace;
layout(location = BILLBOARD_HEIGHT) uniform float u_BBHeight;
layout(location = BILLBOARD_WIDTH) uniform float u_BBWidth;
layout(location = CAMERA_POS_LOC) uniform vec3 u_camPos;

// attributes
layout(location = VERT_POSITION_LOC) in vec3 i_position;
layout(location = VERT_DIFFUSE_TEX_COORD_LOC) in vec2 i_texCoordDiffuse;

// data for fragment shader
out vec2 o_texCoordDiffuse;

void main()
{
  // texture coordinates to fragment shader
  o_texCoordDiffuse = i_texCoordDiffuse;

  vec3 up = u_BBRotAxis;
  vec3 look = u_camPos - u_BBOrigin;
  vec3 right = normalize(cross(u_BBRotAxis, look));
  vec3 front = cross(u_BBRotAxis, right);

  mat4 transMat = mat4(1.0);
  transMat[0].xyz = right;
  transMat[1].xyz = up;
  transMat[2].xyz = front;
  transMat[3].xyz = u_BBOrigin;

  vec3 pos;
  if (u_BBInWorldSpace)
  {
    pos = i_position - u_BBOrigin;
  }
  else
  {
    pos = i_position * vec3(u_BBWidth, u_BBHeight, 0.0);
  }

  // screen space coordinates of the vertex
  gl_Position = u_projMat * u_viewMat * transMat * vec4(pos, 1.f);
}