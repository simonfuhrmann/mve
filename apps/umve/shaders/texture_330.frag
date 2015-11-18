#version 330 core

in vec4 onormal;
smooth in vec2 otexuv;

uniform int lighting = 0;
uniform vec3 light1 = vec3(0.0, 0.0, 5.0);
uniform sampler2D texunit;
uniform vec4 ccolor = vec4(0.7, 0.7, 0.7, 1.0);

layout(location=0) out vec4 frag_color;

void main(void)
{
    vec4 albedo = texture(texunit, otexuv);

    if (lighting == 1)
    {
        /* Compute shading from normal and lights. */
        vec4 light1_hom = vec4(normalize(light1), 0);
        float light_factor = dot(onormal, light1_hom);
        if (light_factor < 0.0)
            light_factor = -light_factor / 2.0;
        frag_color = albedo * light_factor;
    }
    else
    {
        frag_color = albedo;
    }
}
