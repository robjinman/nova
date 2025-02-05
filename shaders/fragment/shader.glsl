#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform Constants {
  layout(offset = 64) bool hasTexture;
} constants;

layout(location = 0) in vec3 inColour;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColour;

void main() {
  const vec3 white = vec3(1, 1, 1);
  vec3 texel = constants.hasTexture ? texture(texSampler, inTexCoord).rgb : white;
  outColour = vec4(inColour * texel, 1.0);
}

