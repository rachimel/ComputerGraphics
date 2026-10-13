#version 460 core

layout (location = 0) in vec2 a_Pos;
layout (location = 1) in vec2 a_TexCoord;

out vec2 TexCoord;

void main()
{
	gl_Position = vec4(a_Pos, 0.0, 1.0);
	TexCoord = a_TexCoord;
}