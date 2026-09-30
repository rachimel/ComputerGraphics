#version 460 core

layout (location = 0) in vec3 a_Pos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 localPos;

void main()
{
	gl_Position = projection * view * model * vec4(a_Pos, 1.0);
	localPos = vec2(a_Pos);
}