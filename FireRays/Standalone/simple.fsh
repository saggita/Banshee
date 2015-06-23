uniform sampler2D g_Texture;

varying vec2 Texcoord;

void main()
{
    vec2 t = vec2(Texcoord.x, 1.0 - Texcoord.y);
    gl_FragColor = texture2D(g_Texture, t);
}