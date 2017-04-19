#version 330 core

in vec4 pos;
in vec3 normal;

out vec4 vpos;
out vec3 vnormal;

void main(void)
{
    vnormal = normal;
    vpos = pos;
}

