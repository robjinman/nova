void main()
{
  vec3 texel = texture(cubeMapSampler, inWorldPos).rgb;
  outColour = vec4(texel, 1.0);
}
