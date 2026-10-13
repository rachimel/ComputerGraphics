#version 460 core

uniform sampler2D hdrTexture;
uniform sampler2D bloomTexture;

uniform float bloomStrength;
uniform float exposure;

in vec2 TexCoord;

out vec4 FragColor;

void main()
{
    vec3 hdrColor = texture(hdrTexture, TexCoord).rgb;
    vec3 bloomColor = texture(bloomTexture, TexCoord).rgb;

    vec3 color = hdrColor + bloomColor * bloomStrength;

    // Tone Mapping
    color = vec3(1.0) - exp(-color * exposure);

    // Gamma Correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}