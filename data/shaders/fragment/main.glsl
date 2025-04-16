#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "fragment/lighting.glsl"
#include "fragment/materials.glsl"

#if defined(FEATURE_TEXTURE_MAPPING) || defined(FEATURE_NORMAL_MAPPING)
layout(location = 0) in vec2 inTexCoord;
#endif
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;
#ifdef FEATURE_NORMAL_MAPPING
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
#endif

layout(location = 0) out vec4 outColour;

#if defined(MAIN_SKYBOX)
#include "fragment/main_skybox.glsl"
#else
#include "fragment/main_default.glsl"
#endif
