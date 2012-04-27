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

in vec4 onormal;
in vec4 ocolor;

uniform vec3 test2;
uniform vec3 test;

void main(void)
{
    /* Enable backface culling. */
    //if (!gl_FrontFacing) discard;

    //gl_FragDepth = gl_FragCoord.z;
    //gl_FragColor = (onormal + vec4(1,1,1,1)) / 2;
    gl_FragColor = ocolor;
    //gl_FragColor = vec4(1, 1, 0, 1);
    //gl_FragColor = vec4(1,0,0,1);
}

