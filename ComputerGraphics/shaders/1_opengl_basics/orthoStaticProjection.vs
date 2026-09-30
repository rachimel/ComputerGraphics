#version 460 core

layout (location = 0) in vec3 a_Pos;

uniform mat4 projection;
uniform mat4 model;

void main()
{
	// view는 생략
	gl_Position = projection * model * vec4(a_Pos, 1.0);
}