#version 460 core
layout (location = 0) in vec3 a_Pos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
	TexCoords = a_Pos;
	gl_Position = projection * view * vec4(TexCoords, 1.0);
}