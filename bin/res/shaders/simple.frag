#version 430

// color for framebuffer
layout(location = FRAG_COLOR_LOC) out vec4 resultingColor;

// material information
layout (std140, binding = MATERIAL_BINDING) uniform Material {
  vec4 u_Kd; // Diffuse reflectivity
  vec4 u_Ka; // Ambient reflectivity
  vec4 u_Ks; // Specular reflectivity
  vec4 u_Ke; // Emissive (currently not used)
  float u_Shininess; // Specular shininess factor
  int u_texCount;
};

// position of light and camera
layout(location = LIGHT_POSITION_LOC) uniform vec3 u_lightDir; // in view space

// directional light information
layout (std140, binding = LIGHT_BINDING) uniform Light {
  vec4 u_La; // Ambient light intensity
  vec4 u_Ld; // Diffuse light intensity
  vec4 u_Ls; // Specular light intensity
};

// texture with diffuse color of the object
layout(binding = DIFFUSE_TEX_UNIT) uniform sampler2D u_diffuseTexture;

// texture with the lightmap color
layout(binding = LIGHTMAP_TEX_UNIT) uniform sampler2D u_lightmapTexture;

// data from vertex shader
in vec3 o_normal;
in vec2 o_texCoordDiffuse;
in vec2 o_texCoordLightmap;

void main()
{
   vec4 vDiffTexColor = texture2D(u_diffuseTexture, o_texCoordDiffuse);
   vec4 vLightmapTexColor = texture2D(u_lightmapTexture, o_texCoordLightmap);
   resultingColor = vDiffTexColor * (vLightmapTexColor + vec4(0.5f));

   if (resultingColor.a < 0.0001) discard;
}

