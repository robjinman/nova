layout(std140, set = 1, binding = 0) uniform MaterialUbo
{
  vec4 colour;
  // TODO: PBR values
} material;

layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D normalMapSampler;
layout(set = 1, binding = 3) uniform samplerCube cubeMapSampler;

vec4 computeTexel(vec2 texCoord)
{
  // TODO: Transparency

  return material.colour * texture(texSampler, texCoord);
}
