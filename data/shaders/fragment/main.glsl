#version 450
#extension GL_ARB_separate_shader_objects : enable

#if defined(FEATURE_TEXTURE_MAPPING) || defined(FEATURE_NORMAL_MAPPING)
layout(location = 0) in vec2 inTexCoord;
#endif
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inLightSpacePos;
#ifdef FEATURE_NORMAL_MAPPING
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;
#endif

#ifdef FEATURE_LIGHTING
#include "fragment/lighting.glsl"
#endif
#ifdef FEATURE_MATERIALS
#include "fragment/materials.glsl"
#endif

#ifndef RENDER_PASS_SHADOW
layout(location = 0) out vec4 outColour;
#endif

#if defined(FRAG_MAIN_SKYBOX)
#include "fragment/main_skybox.glsl"
#elif defined(FRAG_MAIN_DEPTH)
#include "fragment/main_depth.glsl"
#else
#include "fragment/main_default.glsl"
#endif
