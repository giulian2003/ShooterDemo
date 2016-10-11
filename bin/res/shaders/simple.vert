#version 430

layout (std140, binding = MATRICES_BINDING) uniform Matrices {
  mat4 u_projMat;
  mat4 u_viewMat;
  mat4 u_modelMat;
  mat4 u_normalMat; // we only consider the 3x3 matrix
};

layout (std140, binding = BONES_BINDING) uniform Bones {
  mat4 u_bones[MAX_BONES];
};

// attributes
layout(location = VERT_POSITION_LOC) in vec3 i_position;      // xyz - position
layout(location = VERT_NORMAL_LOC) in vec3 i_normal;        // xyz - normal
layout(location = VERT_DIFFUSE_TEX_COORD_LOC) in vec2 i_texCoordDiffuse;    // xy - texture coords
layout(location = VERT_LIGHTMAP_TEX_COORD_LOC) in vec2 i_texCoordLightmap;  // xy - texture coords
layout(location = VERT_BONE_IDS_LOC) in uvec4 i_boneIDs; 
layout(location = VERT_BONE_WEIGHTS_LOC) in vec4 i_weights;

// data for fragment shader
out vec3 o_normal;
out vec2 o_texCoordDiffuse;
out vec2 o_texCoordLightmap;

void main()
{
  mat4 transformMatrix = mat4(1.0);
  for (int i = 0; i < 4; i++) 
  {
    transformMatrix += u_bones[i_boneIDs[i]] * i_weights[i];
  }

  vec4 norm = transformMatrix * vec4(i_normal, 0.0);

  // normal in view space
  o_normal = normalize(mat3(u_normalMat) * norm.xyz);

  // texture coordinates to fragment shader
  o_texCoordDiffuse = i_texCoordDiffuse;
  o_texCoordLightmap = i_texCoordLightmap;

  // screen space coordinates of the vertex
  gl_Position = u_projMat * u_viewMat * u_modelMat * transformMatrix * vec4(i_position, 1.f);
}
