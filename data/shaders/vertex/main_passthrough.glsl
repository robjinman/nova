void main()
{
  vec4 worldPos = vec4(inPos, 1.0);
  gl_Position = camera.projMatrix * mat4(mat3(camera.viewMatrix)) * worldPos;
  outWorldPos = worldPos.xyz;
}
