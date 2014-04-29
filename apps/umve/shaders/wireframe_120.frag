#version 120 

varying vec4 ocolor;

uniform vec4 ccolor = vec4(0.5, 0.5, 0.5, 1.0);

void main(void)
{
    gl_FragDepth = gl_FragCoord.z;
    if (ccolor.a == 0.0)
        gl_FragColor = ocolor;
    else
        gl_FragColor = ccolor;
}
