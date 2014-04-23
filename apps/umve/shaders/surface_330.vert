#version 330 core

/* Vertex shader special variables
 *
 * in  int   gl_VertexID;
 * in  int   gl_InstanceID;
 * out gl_PerVertex {
 *     vec4  gl_Position;
 *     float gl_PointSize;
 *     float gl_ClipDistance[];
 * };
 */

in vec4 pos;
in vec4 color;
in vec3 normal;

out vec4 ocolor;
out vec3 onormal;

uniform mat4 viewmat;
uniform mat4 projmat;

void main(void)
{
    ocolor = color;
    onormal = normal;
    gl_Position = projmat * (viewmat * pos);
}

