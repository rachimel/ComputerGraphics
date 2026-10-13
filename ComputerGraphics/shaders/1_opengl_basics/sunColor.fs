#version 460 core

uniform vec4 sunColor;
uniform vec4 sunlightColor;

uniform int intensity;
in vec3 FragPos;
in vec3 Normal;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

void main()
{
    vec3 viewDir = normalize(-FragPos);
    vec3 normal = normalize(Normal);
    float rimFactor = pow(max(1.0 - dot(normal, viewDir), 0.0), intensity);

    //vec3 baseColor = sunColor.rgb;
    //vec3 rimEmission = sunlightColor.rgb * rimFactor * 2.0;
    //vec3 color = baseColor + rimEmission;

    //FragColor = vec4(color,1.0);

    vec3 baseColor = sunColor.rgb * 1.3;

    vec3 rimColor = sunlightColor.rgb * 2.0;

    // Rim에서 주황색으로 보간
    vec3 color = mix(baseColor, rimColor, rimFactor);

    FragColor = vec4(color, 1.0);

    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    
    BrightColor = brightness > 1.0
        ? vec4(color, 1.0)
        : vec4(0.0, 0.0, 0.0, 1.0);
}