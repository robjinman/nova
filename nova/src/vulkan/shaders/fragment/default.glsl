#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D texSampler;
//layout(set = 1, binding = 0) uniform sampler2D normalMapSampler;

layout(std140, set = 1, binding = 1) uniform MaterialUbo
{
  vec4 colour;
  bool hasTexture;
  //bool hasNormalMap;
} material;

struct Light
{
  vec3 worldPos;
  vec3 colour;
  float ambient;
};

layout(std140, set = 2, binding = 0) uniform LightingUbo
{
  int numLights;
  Light lights[8];
} lighting;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 inWorldPos;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColour;

vec3 computeLight()
{
  vec3 normal = normalize(inNormal);
  vec3 total = vec3(0, 0, 0);

  // TODO: Normal map

  vec3 light = vec3(0, 0, 0);
  for (int i = 0; i < lighting.numLights; ++i) {
    vec3 lightDir = normalize(lighting.lights[i].worldPos - inWorldPos);
    float cosTheta = min(max(dot(normal, lightDir), 0.0), 1.0 - lighting.lights[i].ambient);
    float intensity = lighting.lights[i].ambient + cosTheta;
    light += intensity * lighting.lights[i].colour;
  }

  return light / lighting.numLights;
}

vec3 computeTexel()
{
  // TODO: Transparency

  vec3 texel = material.colour.xyz;

  if (material.hasTexture) {
    texel = texel * texture(texSampler, inTexCoord).rgb;
  }

  return texel;
}

void main()
{
  const vec3 white = vec3(1, 1, 1);

  vec3 light = computeLight();
  vec3 texel = computeTexel();

  outColour = vec4(light * texel, 1.0);
}
