#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject
{
  mat4 viewMatrix;
  mat4 projMatrix;
} ubo;

layout(push_constant) uniform PushConstants
{
  mat4 modelMatrix;
} constants;

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 outWorldPos;

void main()
{
  vec4 worldPos = constants.modelMatrix * vec4(inPos, 1.0);
  gl_Position = ubo.projMatrix * ubo.viewMatrix * worldPos;

  outWorldPos = worldPos.xyz;
}
