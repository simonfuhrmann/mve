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

in vec4 ocolor;

layout(location=0) out vec4 frag_color;

uniform vec4 ccolor = vec4(0.5, 0.5, 0.5, 1.0);

void main(void)
{
    /* Enable backface culling for wireframe. */
    //if (!gl_FrontFacing) discard;

    gl_FragDepth = gl_FragCoord.z;
    if (ccolor.a == 0.0)
        frag_color = ocolor;
    else
        frag_color = ccolor;
}
