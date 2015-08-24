#version 330 core

in vec3 onormal;
smooth in vec2 otexuv;

uniform vec3 light1 = vec3(0.0, 0.0, 5.0);
uniform sampler2D texunit;
uniform vec4 ccolor = vec4(0.7, 0.7, 0.7, 1.0);

layout(location=0) out vec4 frag_color;

void main(void)
{
    frag_color = texture(texunit, otexuv);
}
