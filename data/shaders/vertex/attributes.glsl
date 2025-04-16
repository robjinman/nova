#ifdef ATTR_POSITION
layout(location = 0) in vec3 inPos;
#endif
#ifdef ATTR_NORMAL
layout(location = 1) in vec3 inNormal;
#endif
#ifdef ATTR_TEXCOORD
layout(location = 2) in vec2 inTexCoord;
#endif
#ifdef ATTR_TANGENT
layout(location = 3) in vec3 inTangent;
#endif
#ifdef ATTR_JOINTS
layout(location = 4) in vec4 inJoints;
#endif
#ifdef ATTR_WEIGHTS
layout(location = 5) in vec4 inWeights;
#endif
#ifdef ATTR_MODEL_MATRIX
layout(location = 6) in vec4 inModelMatrix0;
layout(location = 7) in vec4 inModelMatrix1;
layout(location = 8) in vec4 inModelMatrix2;
layout(location = 9) in vec4 inModelMatrix3;
#endif
