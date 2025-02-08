#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform samplerCube texSampler;

layout(location = 0) in vec3 inWorldPos;

layout(location = 0) out vec4 outColour;

void main()
{
  // TODO: In one line?
  vec3 texel = texture(texSampler, inWorldPos).rgb;
  outColour = vec4(texel, 1.0);
}
