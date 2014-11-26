#version 330 core

in vec4 pos;
in vec4 color;

out vec4 ocolor;

uniform mat4 viewmat;
uniform mat4 projmat;

void main(void)
{
    ocolor = color;
    gl_Position = projmat * (viewmat * pos);
}

