#version 450

#include "vertex/attributes.glsl"

layout(std140, set = 0, binding = 0) uniform CameraTransformsUbo
{
  mat4 viewMatrix;
  mat4 projMatrix;
} camera;

layout(std140, set = 0, binding = 1) uniform LightTransformsUbo
{
  mat4 viewMatrix;
  mat4 projMatrix;
} light;

layout(push_constant) uniform PushConstants
{
  mat4 modelMatrix;
} constants;

#if defined(FEATURE_TEXTURE_MAPPING) || defined(FEATURE_NORMAL_MAPPING)
layout(location = 0) out vec2 outTexCoord;
#endif
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outLightSpacePos;
#ifdef FEATURE_NORMAL_MAPPING
layout(location = 4) out vec3 outTangent;
layout(location = 5) out vec3 outBitangent;
#endif

#if defined(VERT_MAIN_PASSTHROUGH)
#include "vertex/main_passthrough.glsl"
#else
#include "vertex/main_default.glsl"
#endif
