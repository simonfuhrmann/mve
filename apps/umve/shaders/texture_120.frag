#version 120

varying vec3 onormal;
varying vec2 otexuv;

uniform vec3 light1 = vec3(0.0, 0.0, 5.0);
uniform sampler2D texunit;

void main(void)
{
    gl_FragDepth = gl_FragCoord.z;
    gl_FragColor = texture2D(texunit, otexuv);
}
