layout(std140, set = DESCRIPTOR_SET_MATERIAL, binding = 0) uniform MaterialUbo
{
  vec4 colour;
  // TODO: PBR values
} material;

layout(set = DESCRIPTOR_SET_MATERIAL, binding = 1) uniform sampler2D texSampler;
layout(set = DESCRIPTOR_SET_MATERIAL, binding = 2) uniform sampler2D normalMapSampler;
layout(set = DESCRIPTOR_SET_MATERIAL, binding = 3) uniform samplerCube cubeMapSampler;

vec4 computeTexel(vec2 texCoord)
{
  return material.colour * texture(texSampler, texCoord);
}
