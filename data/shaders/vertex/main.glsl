#version 450

layout(std140, set = 0, binding = 0) uniform MatricesUbo
{
  mat4 viewMatrix;
  mat4 projMatrix;
} matrices;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
#if defined(TEXTURE_MAPPING) || defined(NORMAL_MAPPING)
layout(location = 2) in vec2 inTexCoord;
#endif
#ifdef NORMAL_MAPPING
layout(location = 3) in vec3 inTangent;
#endif
#ifdef ANIMATED
layout(location = 4) in vec4 inJoints;
layout(location = 5) in vec4 inWeights;
#endif

#ifdef INSTANCED
layout(location = 6) in vec4 inModelMatrix0;
layout(location = 7) in vec4 inModelMatrix1;
layout(location = 8) in vec4 inModelMatrix2;
layout(location = 9) in vec4 inModelMatrix3;
#elif !defined(SKYBOX)
layout(push_constant) uniform PushConstants
{
  mat4 modelMatrix;
} constants;
#endif

#if defined(TEXTURE_MAPPING) || defined(NORMAL_MAPPING)
layout(location = 0) out vec2 outTexCoord;
#endif
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNormal;
#ifdef NORMAL_MAPPING
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec3 outBitangent;
#endif

#ifdef SKYBOX
void skyboxMain()
{
  vec4 worldPos = vec4(inPos, 1.0);
  gl_Position = matrices.projMatrix * mat4(mat3(matrices.viewMatrix)) * worldPos;
  outWorldPos = worldPos.xyz;
}
#else
void defaultMain()
{
#ifdef INSTANCED
  mat4 modelMatrix = mat4(inModelMatrix0, inModelMatrix1, inModelMatrix2, inModelMatrix3);
#else
  mat4 modelMatrix = constants.modelMatrix;
#endif

  vec4 worldPos = modelMatrix * vec4(inPos, 1.0);
  gl_Position = matrices.projMatrix * matrices.viewMatrix * worldPos;

  outWorldPos = worldPos.xyz;

#if defined(TEXTURE_MAPPING) || defined(NORMAL_MAPPING)
  outTexCoord = inTexCoord;
#endif

#ifdef NORMAL_MAPPING
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
#endif

void main()
{
#ifdef SKYBOX
  skyboxMain();
#else
  defaultMain();
#endif
}
