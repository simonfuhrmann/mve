#version 330 core

layout(triangles) in;
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
    vec3 ab =  vpos[1].xyz - vpos[0].xyz;
    vec3 ac =  vpos[2].xyz - vpos[0].xyz;
    vec3 fnormal = normalize(cross(ab, ac));
    vec3 fcentroid = (vpos[0].xyz + vpos[1].xyz + vpos[2].xyz) / 3.0;
    draw_normal(fcentroid, fnormal, len);
}
