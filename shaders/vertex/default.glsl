#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 viewProjectionMatrix;
} ubo;

layout(push_constant) uniform PushConstants {
  mat4 modelMatrix;
} constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outFragColour;
layout(location = 1) out vec2 outFragTexCoord;

void main() {
  gl_Position = ubo.viewProjectionMatrix * constants.modelMatrix * vec4(inPosition, 1.0);
  outFragColour = inColour;
  outFragTexCoord = inTexCoord;
}
