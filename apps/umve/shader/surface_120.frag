#version 120 

uniform mat4 viewmat;
uniform mat4 projmat;
uniform vec4 ccolor = vec4(0.7, 0.7, 0.7, 1.0);
uniform vec3 light1 = vec3(0.0, 0.0, 5.0);
//uniform vec4 ambient = vec4(0.2, 0.2, 0.2, 1.0);

varying vec3 onormal;
varying vec4 ocolor;

void main(void)
{
    /* Assign material albedo. */
    vec4 albedo = ccolor;
    if (ccolor.a == 0.0)
      albedo = ocolor;

    // Compute shading from normal and lights.
    vec4 normal = viewmat * vec4(onormal, 0);
    vec4 light_vector = vec4(normalize(-light1), 0);
    float light_factor = abs(dot(normal, light_vector));

    gl_FragDepth = gl_FragCoord.z;
    gl_FragColor = albedo * light_factor;
    //gl_FragColor = albedo;
}

