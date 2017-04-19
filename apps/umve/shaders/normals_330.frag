#version 330 core

layout(location=0) out vec4 frag_color;
uniform vec4 ccolor = vec4(0.0, 0.5, 0.5, 0.9);

void main(void)
{
    gl_FragDepth = gl_FragCoord.z;
    frag_color = ccolor;
}

