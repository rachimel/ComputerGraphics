#version 460 core

layout (location = 0) in vec3 a_Pos;
out vec4 a_Color;
out vec2 Pos;

uniform float t;
uniform vec2 a_Translation;
void main()
{
	// 원 운동
	vec2 translation = vec2(cos(t), sin(t)) * 0.3 * sin(t);
	vec3 ripplePos = a_Pos;
	gl_Position = vec4(ripplePos.xy + translation + a_Translation, ripplePos.z, 1.0);
	Pos = vec2(a_Pos);
}