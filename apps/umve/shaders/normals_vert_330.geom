#version 330 core

layout(points) in;
layout(line_strip, max_vertices = 2) out;

in vec4 vpos[];
in vec3 vnormal[];

uniform mat4 viewmat;
uniform mat4 projmat;
uniform float len = 0.01; // normal length

void draw_normal(vec3 pos, vec3 direction, float len)
{
    gl_Position = projmat * (viewmat * vec4(pos, 1.0));
    EmitVertex();

    vec3 end = pos + direction * len;
    gl_Position = projmat * (viewmat * vec4(end, 1.0));
    EmitVertex();

    EndPrimitive();
}

void main(void)
{
    vec3 pos = vpos[0].xyz;
    vec3 normal = normalize(vnormal[0]);
    draw_normal(pos, normal, len);
}
