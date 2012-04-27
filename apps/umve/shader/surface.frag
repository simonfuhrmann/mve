#version 150 core

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

uniform mat4 viewmat;
uniform mat4 projmat;
uniform vec4 ccolor = vec4(0.7, 0.7, 0.7, 1.0);
uniform vec3 light1 = vec3(0.0, 0.0, 5.0);

void main(void)
{
    /* Assign material albedo. */
    vec4 albedo = ccolor;
    if (ccolor.a == 0.0)
        albedo = ocolor;

    /* Enable backface culling. */
    if (!gl_FrontFacing)
    {
        albedo = albedo * 0.2;
    }

    vec4 normal = viewmat * vec4(onormal, 0);
    vec4 light_vector = vec4(normalize(-light1), 0);
    float light_factor = abs(dot(normal, light_vector));

    //gl_FragDepth = gl_FragCoord.z;
    gl_FragColor = albedo;// * light_factor;
}

