#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalMapSampler;
layout(set = 1, binding = 3) uniform samplerCube cubeMapSampler;

layout(location = 0) in vec3 inWorldPos;

layout(location = 0) out vec4 outColour;

void main()
{
  vec3 texel = texture(cubeMapSampler, inWorldPos).rgb;
  outColour = vec4(texel, 1.0);
}
