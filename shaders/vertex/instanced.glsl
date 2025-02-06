#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 viewProjectionMatrix;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColour;
layout(location = 3) in vec2 inTexCoord;

layout(location = 4) in vec4 inModelMatrix0;
layout(location = 5) in vec4 inModelMatrix1;
layout(location = 6) in vec4 inModelMatrix2;
layout(location = 7) in vec4 inModelMatrix3;

layout(location = 0) out vec3 outFragColour;
layout(location = 1) out vec2 outFragTexCoord;

void main() {
  mat4 modelMatrix = mat4(inModelMatrix0, inModelMatrix1, inModelMatrix2, inModelMatrix3);
  gl_Position = ubo.viewProjectionMatrix * modelMatrix * vec4(inPosition, 1.0);
  outFragColour = inColour;
  outFragTexCoord = inTexCoord;
}

