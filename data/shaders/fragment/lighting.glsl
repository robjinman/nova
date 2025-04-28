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

layout(set = 3, binding = 0) uniform sampler2D shadowMapSampler;

vec3 computeLight(vec3 worldPos, vec3 normal)
{
  vec3 light = vec3(0, 0, 0);

  for (int i = 0; i < lighting.numLights; ++i) {
    float shadow = 1.0;

    // TODO: Currently, only the first light casts shadows
    if (i == 0) {
      vec3 lightSpacePos = (inLightSpacePos / inLightSpacePos.w).xyz;
      vec2 lightSpaceXy = lightSpacePos.xy * 0.5 + 0.5;
      float minDistanceFromLight = texture(shadowMapSampler, lightSpaceXy).r;

      if (lightSpacePos.z > minDistanceFromLight) {
        shadow = 0.0;
      }
    }

    float intensity = lighting.lights[i].ambient;

    vec3 lightDir = normalize(lighting.lights[i].worldPos - worldPos);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 viewDir = normalize(lighting.cameraPos - worldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float specular = lighting.lights[i].specular * pow(max(dot(viewDir, reflectDir), 0.0), 32);
    intensity += shadow * (diffuse + specular);

    light += intensity * lighting.lights[i].colour;
  }

  return light;
}
