#version 460 core

layout (location = 0) in vec3 a_Pos;

uniform mat4 model;
uniform mat4 projection;

// 카메라는 항상 (0,0)에 고정이므로 view는 무시

void main()
{
	gl_Position = projection * model * vec4(a_Pos, 1.0);
}