#version 450

layout(std140, set = 0, binding = 0) uniform MatricesUbo
{
  mat4 viewMatrix;
  mat4 projMatrix;
} matrices;

layout(push_constant) uniform PushConstants
{
  mat4 modelMatrix;
} constants;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNormal;

void main()
{
  vec4 worldPos = constants.modelMatrix * vec4(inPos, 1.0);
  gl_Position = matrices.projMatrix * matrices.viewMatrix * worldPos;

  outTexCoord = inTexCoord;
  outWorldPos = worldPos.xyz;
  // TODO: Compute normal matrix on CPU
  outNormal = mat3(transpose(inverse(constants.modelMatrix))) * inNormal;
}
