uniform mat4 g_mWorldViewProj;
attribute vec3 inPosition;

varying   float aDepth; 

void main()
{
	gl_Position = vec4(inPosition, 1.0) * g_mWorldViewProj;
	aDepth = gl_Position.w / 10.f;
}