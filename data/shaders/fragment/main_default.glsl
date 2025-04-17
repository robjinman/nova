void main()
{
#ifdef FEATURE_NORMAL_MAPPING
  vec3 tangentSpaceNormal = normalize(texture(normalMapSampler, inTexCoord).rgb * 2.0 - 1.0);
  mat3 tbn = mat3(inTangent, inBitangent, inNormal);
  vec3 normal = normalize(tbn * tangentSpaceNormal);
#else
  vec3 normal = normalize(inNormal);
#endif

  vec3 light = computeLight(inWorldPos, normal);

#ifdef FEATURE_TEXTURE_MAPPING
  vec4 texel = computeTexel(inTexCoord);
#else
  vec4 texel = material.colour;
#endif

  if (texel.a < 0.5) {
    discard;
  }
  else {
    outColour = vec4(light, 1.0) * texel;
  }
}
