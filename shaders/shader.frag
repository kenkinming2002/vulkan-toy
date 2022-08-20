#version 450

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 light_pos   = vec3(0.0, 10.0, 0.0);

  vec3 normal    = normalize(fragNormal);
  vec3 light_dir = normalize(light_pos - fragPos);

  float ambient_strength = 0.0;
  float diffuse_strength = max(dot(fragNormal, light_dir), 0.0);

  //outColor = vec4(fragColor, 1.0);
  //outColor = vec4((ambient_strength + diffuse_strength) * texture(texSampler, fragUV).rgb * fragColor, 1.0);
  outColor = vec4(texture(texSampler, fragUV).rgb * fragColor, 1.0);
}
