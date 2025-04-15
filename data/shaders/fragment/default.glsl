#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, set = 1, binding = 0) uniform MaterialUbo
{
  vec4 colour;
  // TODO: PBR values
} material;

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalMapSampler;
layout(set = 1, binding = 3) uniform samplerCube cubeMapSampler;

struct Light
{
  vec3 worldPos;
  vec3 colour;
  float ambient;
  float specular;
};

layout(std140, set = 2, binding = 0) uniform LightingUbo
{
  vec3 cameraPos;
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

  vec3 light = vec3(0, 0, 0);
  for (int i = 0; i < lighting.numLights; ++i) {
    vec3 lightDir = normalize(lighting.lights[i].worldPos - inWorldPos);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 viewDir = normalize(lighting.cameraPos - inWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float specular = lighting.lights[i].specular * pow(max(dot(viewDir, reflectDir), 0.0), 32);
    float intensity = lighting.lights[i].ambient + diffuse + specular;
    light += intensity * lighting.lights[i].colour;
  }

  return light;
}

vec4 computeTexel()
{
  // TODO: Transparency

  return material.colour * texture(texSampler, inTexCoord);
}

void main()
{
  const vec3 white = vec3(1, 1, 1);

  vec3 light = computeLight();
  vec4 texel = computeTexel();

  outColour = vec4(light, 1.0) * texel;
}
