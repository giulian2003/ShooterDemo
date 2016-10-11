#version 430

layout (std140, binding = MATRICES_BINDING) uniform Matrices {
  mat4 u_projMat;
  mat4 u_viewMat;
  mat4 u_modelMat;
  mat4 u_normalMat; // we only consider the 3x3 matrix
};

// attributes
layout(location = VERT_POSITION_LOC) in vec3 i_posNDC;

smooth out vec3 eyeDirection;

void main() 
{
    mat4 invProjMat = inverse(u_projMat);
    mat3 invViewMat = transpose(mat3(u_viewMat)); // use the transpose, because we have no translation
    vec4 posView = invProjMat * vec4(i_posNDC, 1.f);
    eyeDirection = invViewMat * (posView.xyz / posView.w);

    gl_Position = vec4(i_posNDC, 1.f);
} 