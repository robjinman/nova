#version 450

layout(std140, set = 0, binding = 0) uniform MatricesUbo
{
  mat4 viewMatrix;
  mat4 projMatrix;
} matrices;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 3) in vec4 inModelMatrix0;
layout(location = 4) in vec4 inModelMatrix1;
layout(location = 5) in vec4 inModelMatrix2;
layout(location = 6) in vec4 inModelMatrix3;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNormal;

void main()
{
  mat4 modelMatrix = mat4(inModelMatrix0, inModelMatrix1, inModelMatrix2, inModelMatrix3);
  vec4 worldPos = modelMatrix * vec4(inPos, 1.0);
  gl_Position = matrices.projMatrix * matrices.viewMatrix * worldPos;

  outTexCoord = inTexCoord;
  outWorldPos = worldPos.xyz;
  outNormal = inNormal;
}
