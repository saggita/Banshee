uniform mat4 g_mWorldViewProj;

attribute vec3 inPosition;
attribute vec2 inTexcoord;
attribute vec3 inNormal;

varying   vec3 WorldPos;
varying   vec3 Normal;

void main()
{
	gl_Position = vec4(inPosition, 1.0) * g_mWorldViewProj;
	WorldPos = inPosition;
	Normal = inNormal;
}