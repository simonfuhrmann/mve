#version 330

in vec3 onormal;
smooth in vec2 otexuv;

//uniform mat4 viewmat;
//uniform mat4 projmat;
uniform vec3 light1 = vec3(0.0, 0.0, 5.0);
uniform sampler2D texunit;

void main(void)
{
    //gl_FragDepth = gl_FragCoord.z;
    //gl_FragColor = vec4(otexuv.x, otexuv.y, 0.0f, 1.0f);
    gl_FragColor = texture(texunit, otexuv);

#if 0
    vec4 normal = viewmat * vec4(onormal, 0);
    vec4 light_vector = vec4(normalize(-light1), 0);
    float light_factor = abs(dot(normal, light_vector));

    //gl_FragDepth = gl_FragCoord.z;
    gl_FragColor = texture(texunit, otexuv) * light_factor;
#endif
}
