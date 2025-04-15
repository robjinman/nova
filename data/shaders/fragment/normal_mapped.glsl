#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "fragment/lighting.glsl"
#include "fragment/materials.glsl"

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outColour;

void main()
{
  vec3 tangentSpaceNormal = normalize(texture(normalMapSampler, inTexCoord).rgb * 2.0 - 1.0);
  mat3 tbn = mat3(inTangent, inBitangent, inNormal);
  vec3 normal = normalize(tbn * tangentSpaceNormal);

  vec3 light = computeLight(inWorldPos, normal);
  vec4 texel = computeTexel(inTexCoord);

  outColour = vec4(light, 1.0) * texel;
}
