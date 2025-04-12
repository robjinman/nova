#version 450

layout(std140, set = 0, binding = 0) uniform MatricesUbo
{
  mat4 viewMatrix;
  mat4 projMatrix;
  // TODO: Normal matrix
} matrices;

layout(push_constant) uniform PushConstants
{
  mat4 modelMatrix;
} constants;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out mat3 outTbn;

void main()
{
  vec4 worldPos = constants.modelMatrix * vec4(inPos, 1.0);
  gl_Position = matrices.projMatrix * matrices.viewMatrix * worldPos;

  vec3 T = normalize(mat3(constants.modelMatrix) * inTangent);
  vec3 N = normalize(mat3(constants.modelMatrix) * inNormal);
  vec3 B = normalize(cross(T, N)); // TODO: Need to normalize?

  outTbn = mat3(T, B, N);
  outTexCoord = inTexCoord;
  outWorldPos = worldPos.xyz;
}
