#version 450 core

layout (quads, equal_spacing, cw) in;

layout (std140, binding = MATRICES_BINDING) uniform Matrices {
  mat4 u_projMat;
  mat4 u_viewMat;
  mat4 u_modelMat;
  mat4 u_normalMat; // we only consider the 3x3 matrix
};

in vec3 tes_o_normal[];
in vec2 tes_o_texCoordDiffuse[];
in vec2 tes_o_texCoordLightmap[];

out vec3 o_normal;
out vec2 o_texCoordDiffuse;
out vec2 o_texCoordLightmap;


vec2 quadratic_bezier_vec2(vec2 A, vec2 B, vec2 C, float t)
{
    vec4 D = mix(vec4(A,B), vec4(B,C), t);

    return mix(D.xy, D.zw, t);
}

vec4 quadratic_bezier_vec4(vec4 A, vec4 B, vec4 C, float t)
{
    vec4 D = mix(A, B, t);
    vec4 E = mix(B, C, t);

    return mix(D, E, t);
}

vec2 evaluate_texCoordDiffuse(vec2 at)
{
    vec2 P[3];
    int i;

    for (i = 0; i < 3; i++)
    {
      P[i] = quadratic_bezier_vec2(
        tes_o_texCoordDiffuse[i + 0],
        tes_o_texCoordDiffuse[i + 3],
        tes_o_texCoordDiffuse[i + 6],
        at.y);
    }

    return quadratic_bezier_vec2(P[0], P[1], P[2], at.x);
}

vec2 evaluate_texCoordLightmap(vec2 at)
{
    vec2 P[3];
    int i;

    for (i = 0; i < 3; i++)
    {
      P[i] = quadratic_bezier_vec2(
        tes_o_texCoordLightmap[i + 0],
        tes_o_texCoordLightmap[i + 3],
        tes_o_texCoordLightmap[i + 6],
        at.y);
    }

    return quadratic_bezier_vec2(P[0], P[1], P[2], at.x);
}

vec4 evaluate_positions(vec2 at)
{
    vec4 P[3];
    int i;

    for (i = 0; i < 3; i++)
    {
      P[i] = quadratic_bezier_vec4(
        gl_in[i + 0].gl_Position,
        gl_in[i + 3].gl_Position,
        gl_in[i + 6].gl_Position,
        at.y);
    }

    return quadratic_bezier_vec4(P[0], P[1], P[2], at.x);
}

const float epsilon = 0.001;

void main(void)
{
  vec4 p1 = evaluate_positions(gl_TessCoord.xy);
  vec4 p2 = evaluate_positions(gl_TessCoord.xy + vec2(0.0, epsilon));
  vec4 p3 = evaluate_positions(gl_TessCoord.xy + vec2(epsilon, 0.0));

  vec3 v1 = normalize(p2.xyz - p1.xyz);
  vec3 v2 = normalize(p3.xyz - p1.xyz);
  o_normal = cross(v1, v2);

  o_texCoordDiffuse = evaluate_texCoordDiffuse(gl_TessCoord.xy);
  o_texCoordLightmap = evaluate_texCoordLightmap(gl_TessCoord.xy);

  gl_Position = u_projMat * u_viewMat * u_modelMat * p1;
}