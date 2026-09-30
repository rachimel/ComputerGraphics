#version 460 core

in vec2 Pos;

out vec4 FragColor;

uniform vec4 a_Color;
uniform float t;
void main()
{
    // 반지름
    float r = length(Pos);
    // pow 안의 함수가 변경되면 줌인, 줌아웃 효과를 낼 수 있음
    float k = sin(pow(0.5 * sin(t) + 1, sin(t))) * 100.0f;

    float wave = sin(r * k);
    // 색상 경계 [0, 1]로 바꾸기 위해 보정
    wave = wave * 0.5 + 0.5;

    // clamp의 첫번째 인자를 조절해서 두께 조절 가능
    float intensity = 0.3 + clamp(0.7 * wave * 2,0.0, 1.0);
    // 색과 합성
    FragColor = a_Color * intensity;
}