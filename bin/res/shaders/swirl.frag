#version 430

// color for framebuffer
layout(location = FRAG_COLOR_LOC) out vec4 resultingColor;

// time in seconds
layout(location = GLOBAL_TIME_LOC) uniform float u_globalTime;

// texture with diffuse color of the object
layout(binding = DIFFUSE_TEX_UNIT) uniform sampler2D u_diffuseTexture;

// data from vertex shader
in vec2 o_texCoordDiffuse;

// Swirl Shader
// https://www.shadertoy.com/view/XdlSWB

void main()
{
  vec2 center = vec2(0.5,0.5);
  vec2 tc = 0.7 * (o_texCoordDiffuse - center);
  
  float radius = 0.5;
  float dist = length(tc);
  float percent = 1.0 - dist / radius;
  float theta = u_globalTime * (1.0 + 0.008 * percent * percent);
  
  // rotate tc by theta around center
  float s = sin(theta);
  float c = cos(theta);
  tc = vec2(dot(tc, vec2(c, -s)), dot(tc, vec2(s, c)));

  tc += center;
  vec3 color = texture2D(u_diffuseTexture, tc).rgb;
  resultingColor = vec4(color, 2.0 * percent);
}
