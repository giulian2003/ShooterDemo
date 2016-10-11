#version 450

// color for framebuffer
layout(location = FRAG_COLOR_LOC) out vec4 resultingColor;

// texture with diffuse color of the object
layout(binding = DIFFUSE_TEX_UNIT) uniform sampler2D u_diffuseTexture;

// texture with the lightmap color
layout(binding = LIGHTMAP_TEX_UNIT) uniform sampler2D u_lightmapTexture;

// data from vertex shader
in vec3 o_normal;
in vec2 o_texCoordDiffuse;
in vec2 o_texCoordLightmap;

const vec3 sunColor = vec3(1.f); // white 
const vec3 sunDirection = vec3(1.f, 1.f, -1.f);
const float sunAmbientIntensity = 0.4f; 

void main()
{
   vec4 vDiffTexColor = texture2D(u_diffuseTexture, o_texCoordDiffuse);
   vec4 vLightmapTexColor = texture2D(u_lightmapTexture, o_texCoordLightmap); 
   resultingColor = vDiffTexColor;
}

