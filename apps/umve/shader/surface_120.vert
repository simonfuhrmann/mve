#version 120

attribute vec4 pos;
attribute vec4 color;
attribute vec3 normal;

uniform mat4 viewmat;
uniform mat4 projmat;

varying vec3 onormal;
varying vec4 ocolor;

void main()
{
	ocolor = color;
    onormal = normal; 
	gl_Position = projmat * (viewmat * pos);
}
