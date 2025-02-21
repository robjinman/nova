#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_LIGHTS 8;

struct Light
{
  vec3 worldPos;
  vec3 colour;
  float ambient;
};

layout(set = 1, binding = 0) uniform sampler2D texSampler;
//layout(set = 1, binding = 0) uniform sampler2D normalMapSampler;

layout(set = 1, binding = 1) uniform MaterialData
{
  vec3 colour;
  bool hasTexture;
  //bool hasNormalMap;
} material;

layout(set = 2, binding = 0) uniform LightData
{
  Light[MAX_LIGHTS] lights;
  int numLights;
} lighting;

layout(location = 0) in vec3 inColour;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inWorldPos;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec4 outColour;

vec3 computeLight()
{
  vec3 normal = normalize(inNormal);
  vec3 total = vec3(0, 0, 0);

  // TODO: Normal map

  for (int i = 0; i < lighting.numLights; ++i) {
    vec3 lightDir = normalize(lighting.lights[i].worldPos - inWorldPos);
    float cosTheta = min(max(dot(normal, lightDir), 0.0), 1.0 - lighting.lights[i].ambient);
    float intensity = lighting.lights[i].ambient + cosTheta;
    light = light + intensity * lighting.lights[i].colour;
  }

  return light / lighting.numLights;
}

vec3 computeTexel()
{
  vec3 texel = material.colour;

  if (material.hasTexture) {
    texel = texel * texture(material.texSampler, inTexCoord).rgb;
  }

  return texel;
}

void main()
{
  const vec3 white = vec3(1, 1, 1);

  vec3 light = computeLight();
  vec3 texel = computeTexel();

  outColour = vec4(inColour * light * texel, 1.0);
}
