void main()
{
  vec4 worldPos = vec4(inPos, 1.0);
  gl_Position = matrices.projMatrix * mat4(mat3(matrices.viewMatrix)) * worldPos;
  outWorldPos = worldPos.xyz;
}
