#version 460 core

uniform vec3 quadColor;

out vec4 FragColor;

void main()
{
    FragColor = vec4(quadColor, 1.0);
}