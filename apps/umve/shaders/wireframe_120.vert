#version 120 

attribute vec4 pos;
attribute vec4 color;

varying vec4 ocolor;

uniform mat4 viewmat;
uniform mat4 projmat;

void main(void)
{
    ocolor = color;
    gl_Position = projmat * (viewmat * pos);
}

