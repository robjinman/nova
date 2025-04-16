#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifndef SKYBOX
#include "fragment/lighting.glsl"
#endif
#include "fragment/materials.glsl"

#if defined(TEXTURE_MAPPING) || defined(NORMAL_MAPPING)
layout(location = 0) in vec2 inTexCoord;
#endif
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;
#ifdef NORMAL_MAPPING
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
#endif

layout(location = 0) out vec4 outColour;

void main()
{
#ifdef NORMAL_MAPPING
  vec3 tangentSpaceNormal = normalize(texture(normalMapSampler, inTexCoord).rgb * 2.0 - 1.0);
  mat3 tbn = mat3(inTangent, inBitangent, inNormal);
  vec3 normal = normalize(tbn * tangentSpaceNormal);
#else
  vec3 normal = normalize(inNormal);
#endif

#ifdef SKYBOX
  vec3 texel = texture(cubeMapSampler, inWorldPos).rgb;
  outColour = vec4(texel, 1.0);
#else
  vec3 light = computeLight(inWorldPos, normal);

#ifdef TEXTURE_MAPPING
  vec4 texel = computeTexel(inTexCoord);
#else
  vec4 texel = material.colour;
#endif

  outColour = vec4(light, 1.0) * texel;
#endif
}
