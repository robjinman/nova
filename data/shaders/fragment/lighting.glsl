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

vec3 computeLight(vec3 worldPos, vec3 normal)
{
  vec3 light = vec3(0, 0, 0);
  for (int i = 0; i < lighting.numLights; ++i) {
    vec3 lightDir = normalize(lighting.lights[i].worldPos - worldPos);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 viewDir = normalize(lighting.cameraPos - worldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float specular = lighting.lights[i].specular * pow(max(dot(viewDir, reflectDir), 0.0), 32);
    float intensity = lighting.lights[i].ambient + diffuse + specular;
    light += intensity * lighting.lights[i].colour;
  }

  return light;
}
