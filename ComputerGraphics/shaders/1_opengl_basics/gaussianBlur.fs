#version 460 core

in vec2 TexCoord;

uniform sampler2D screenTexture;
uniform bool blurHorizontal;
out vec4 FragColor;


// gaussian kernel table
// obtained from
// https://demofox.org/gauss.html
// where sigma = 1.0, support = 0.995

const float gaussianWeights[5] = float[](
    0.3829,
    0.2417,
    0.0606,
    0.0060,
    0.0002
);

// sigma = 1.8
const float moreGaussianWeights[8] = float[](
    0.1974,
    0.1747,
    0.1210,
    0.0656,
    0.0278,
    0.0092,
    0.0024,
    0.0005
);
void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(screenTexture, 0));
    // vec3 result = texture(screenTexture, TexCoord).rgb * gaussianWeights[0];

    //for(int i = 1; i < 5; ++i)
    //{
    //    vec2 direction = blurHorizontal ? vec2(texelSize.x * i, 0.0) : vec2(0.0, texelSize.y);
    //    vec2 offset = direction * 4.0;
    //    result += texture(screenTexture, TexCoord + offset).rgb * gaussianWeights[i];
    //    result += texture(screenTexture, TexCoord - offset).rgb * gaussianWeights[i];
    //}
    vec3 result = texture(screenTexture, TexCoord).rgb * moreGaussianWeights[0];
    for(int i = 1; i < 8; ++i)
    {
        vec2 direction = blurHorizontal ? vec2(texelSize.x * i, 0.0) : vec2(0.0, texelSize.y * i);
        vec2 offset = direction * 2.0;
        result += texture(screenTexture, TexCoord + offset).rgb * moreGaussianWeights[i];
        result += texture(screenTexture, TexCoord - offset).rgb * moreGaussianWeights[i];
    }
    FragColor = vec4(result, 1.0);
}