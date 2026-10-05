#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec4 VertexColor;

void main()
{
	gl_Position = projection * view * model * vec4(a_Pos, 1.0);
	VertexColor = vec4(a_Color, 1.0);
}
