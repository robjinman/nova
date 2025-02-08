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
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColour;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outColour;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outWorldPos;
layout(location = 3) out vec3 outNormal;

void main()
{
  vec4 worldPos = constants.modelMatrix * vec4(inPos, 1.0);
  gl_Position = ubo.projMatrix * ubo.viewMatrix * worldPos;

  outColour = inColour;
  outTexCoord = inTexCoord;
  outWorldPos = worldPos.xyz;
  // TODO: Compute normal matrix on CPU
  outNormal = mat3(transpose(inverse(constants.modelMatrix))) * inNormal;
}
