#version 330 core

/* Fragment Shader Special Variables
 *
 * in  vec4  gl_FragCoord;
 * in  bool  gl_FrontFacing;
 * in  float gl_ClipDistance[];
 * out vec4  gl_FragColor;                   // deprecated
 * out vec4  gl_FragData[gl_MaxDrawBuffers]; // deprecated
 * out float gl_FragDepth;
 * in  vec2  gl_PointCoord;
 * in  int   gl_PrimitiveID;
 */

in vec3 onormal;
in vec4 ocolor;

layout(location=0) out vec4 frag_color;

uniform mat4 viewmat;
uniform mat4 projmat;
uniform vec4 ccolor = vec4(0.7, 0.7, 0.7, 1.0);

uniform int lighting = 0;
uniform vec3 light1 = vec3(0.0, 0.0, 5.0);

void main(void)
{
    /* Assign material albedo. */
    vec4 albedo = ccolor;
    if (ccolor.a == 0.0)
        albedo = ocolor;

    /* Enable backface culling. */
    //if (!gl_FrontFacing)
    //    albedo = albedo * 0.2;

    gl_FragDepth = gl_FragCoord.z;
    if (lighting == 1)
    {
        /* Compute shading from normal and lights. */
        vec4 normal = viewmat * vec4(onormal, 0);
        vec4 light_vector = vec4(normalize(light1), 0);
        float light_factor = dot(normal, light_vector);
        if (light_factor < 0.0)
            light_factor = -light_factor / 2.0;
        frag_color = albedo * light_factor;
    }
    else
    {
        frag_color = albedo;
    }
}

