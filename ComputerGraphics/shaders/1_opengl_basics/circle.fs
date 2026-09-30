#version 460 core

uniform vec4 a_Color;

in vec2 localPos;

out vec4 FragPos;
void main()
{
    if(dot(localPos, localPos) > 0.25)
        discard;

    FragPos = a_Color;
}