#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "fragment/lighting.glsl"
#include "fragment/materials.glsl"

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColour;

void main()
{
  vec3 light = computeLight(inWorldPos, normalize(inNormal));
  vec4 texel = material.colour;

  outColour = vec4(light, 1.0) * texel;
}
