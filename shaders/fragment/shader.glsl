#version 450
#extension GL_ARB_separate_shader_objects : enable

// TODO: Pass these in
const vec3 worldLightPos = vec3(150, 100, 400);
const vec3 lightColour = vec3(1, 1, 1);
const float ambient = 0.1;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform Constants {
  layout(offset = 64) bool hasTexture;
} constants;

layout(location = 0) in vec3 inColour;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inWorldPos;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec4 outColour;

void main() {
  vec3 normal = normalize(inNormal);
  vec3 lightDir = normalize(worldLightPos - inWorldPos);
  float cosTheta = min(max(dot(normal, lightDir), 0.0), 1.0 - ambient);
  vec3 light = ambient + cosTheta * lightColour;

  const vec3 white = vec3(1, 1, 1);
  vec3 texel = constants.hasTexture ? texture(texSampler, inTexCoord).rgb : white;

  outColour = vec4(inColour * light * texel, 1.0);
}
