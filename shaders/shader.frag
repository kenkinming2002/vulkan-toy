#version 450

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main()
{
  float light_intensity = 50.0f;
  vec3  light_pos       = vec3(0.0, 2.0, 2.0);
  vec3  light_color     = vec3(0.2, 0.2, 0.8);

  vec3 ambient_color  = texture(texSampler, fragUV).rgb * fragColor;
  vec3 diffuse_color  = texture(texSampler, fragUV).rgb * fragColor;
  vec3 specular_color = texture(texSampler, fragUV).rgb * fragColor;

  vec3  light_dir      = normalize(light_pos - fragPos);
  float light_distance = length(light_pos - fragPos);
  vec3  normal         = normalize(fragNormal);

  float cos_factor    = clamp(dot(fragNormal, light_dir), 0.0, 1.0);
  float light_factor = light_intensity / (light_distance * light_distance);

  float ambient_strength  = 0.0;
  float diffuse_strength  = cos_factor * light_factor;
  float specular_strength = pow(cos_factor, 5)  * light_factor;

  vec3 result_color =
    ambient_color * ambient_strength +
    diffuse_color * diffuse_strength * light_color +
    specular_color * specular_strength * light_color;

  outColor = vec4(result_color , 1.0);
}
