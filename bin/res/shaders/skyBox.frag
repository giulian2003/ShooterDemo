#version 430

// color for framebuffer
layout(location = FRAG_COLOR_LOC) out vec4 resultingColor;

// texture with diffuse color of the object
layout(binding = DIFFUSE_TEX_UNIT) uniform samplerCube u_skyBoxTexture;

smooth in vec3 eyeDirection;

void main() 
{
    resultingColor = texture(u_skyBoxTexture, eyeDirection);
}
