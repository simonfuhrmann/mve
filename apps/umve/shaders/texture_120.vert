#version 120

attribute vec4 pos;
attribute vec3 normal;
attribute vec2 texuv;

varying vec3 onormal;
varying vec2 otexuv;

uniform mat4 viewmat;
uniform mat4 projmat;

void main(void)
{
    onormal = normal;
    otexuv = texuv;
    gl_Position = pos;
}
