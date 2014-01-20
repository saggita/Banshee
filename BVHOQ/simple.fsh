uniform sampler2D g_Texture;

varying vec2 Texcoord;

void main()
{
	gl_FragColor = texture2D(g_Texture, Texcoord);// * 0.04;
}