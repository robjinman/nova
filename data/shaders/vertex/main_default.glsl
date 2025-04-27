void main()
{
#ifdef ATTR_MODEL_MATRIX
  mat4 modelMatrix = mat4(inModelMatrix0, inModelMatrix1, inModelMatrix2, inModelMatrix3);
#else
  mat4 modelMatrix = constants.modelMatrix;
#endif

  vec4 worldPos = modelMatrix * vec4(inPos, 1.0);
  gl_Position = camera.projMatrix * camera.viewMatrix * worldPos;

  outWorldPos = worldPos.xyz;
  outLightSpacePos = (light.projMatrix * light.viewMatrix * worldPos).xyz;

#if defined(FEATURE_TEXTURE_MAPPING) || defined(FEATURE_NORMAL_MAPPING)
  outTexCoord = inTexCoord;
#endif

#ifdef FEATURE_NORMAL_MAPPING
  vec3 T = normalize(mat3(modelMatrix) * normalize(inTangent));
  vec3 N = normalize(mat3(modelMatrix) * normalize(inNormal));
  T = normalize(T - dot(T, N) * N);
  vec3 B = normalize(cross(N, T));
  outTangent = T;
  outBitangent = B;
  outNormal = N;
#else
  outNormal = mat3(modelMatrix) * inNormal;
#endif
}
