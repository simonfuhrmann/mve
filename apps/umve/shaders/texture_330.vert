#version 330 core

in vec4 pos;
in vec3 normal;
in vec2 texuv;

out vec4 onormal;
smooth out vec2 otexuv;

uniform mat4 viewmat;
uniform mat4 projmat;

void main(void)
{
    onormal = viewmat * vec4(normal, 0);
    otexuv = texuv;
    gl_Position = projmat * (viewmat * pos);
}
