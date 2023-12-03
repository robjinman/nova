#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 inColour;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColour;

void main() {
  outColour = vec4(inColour * texture(texSampler, inTexCoord).rgb, 1.0);
}

