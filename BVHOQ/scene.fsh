#define POINT_LIGHTS_MAX 4
#define SPOT_LIGHTS_MAX 4

#define SPECULAR_EXPONENT 32

varying vec3 Normal;
varying vec3 WorldPos;

struct PointLight
{
	vec4 vPos;
	vec4 vColor;
};

struct SpotLight
{
	vec4 vPos;
	vec4 vDir;
	vec4 vColor;
	vec4 vAngle;
};

uniform PointLight g_PointLights[POINT_LIGHTS_MAX];
uniform SpotLight  g_SpotLights[SPOT_LIGHTS_MAX];
uniform vec3	   g_vEyePos;

void main() 
{
	vec4 vResult = vec4(0.0,0.0,0.0,0.0);
	vec3 vNorm = normalize(Normal);
	vec3 vEye = normalize(g_vEyePos - WorldPos);

	for (int i=0;i<POINT_LIGHTS_MAX;++i)
	{
			vec3 vDir = normalize(g_PointLights[i].vPos.xyz - WorldPos);
			float fDiffuse = max(0.0, dot(vNorm, vDir));
			vec3 vHalf = normalize(vDir + vEye);
			float fSpec = pow(max(0, dot(vHalf, vNorm)), SPECULAR_EXPONENT);
			vResult += (fDiffuse * g_PointLights[i].vColor + fSpec * vec4(1,1,1,1)) * g_PointLights[i].vPos.w;
	}

	for (int i=0;i<SPOT_LIGHTS_MAX;++i)
	{
			vec3 vDir = normalize(g_SpotLights[i].vPos.xyz - WorldPos);
			vec3 vSpotAxis = normalize(g_SpotLights[i].vDir.xyz);
			float fAngularAttenuation = smoothstep(g_SpotLights[i].vAngle.y, g_SpotLights[i].vAngle.x, max(0.0, dot(vSpotAxis, -vDir)));
			float fDiffuse = max(0.0, dot(vNorm, vDir));
			vec3 vHalf = normalize(vDir + vEye);
			float fSpec = pow(max(0, dot(vHalf, vNorm)), SPECULAR_EXPONENT);
			vResult += (fDiffuse * g_SpotLights[i].vColor + fSpec * vec4(1,1,1,1)) * fAngularAttenuation * g_SpotLights[i].vPos.w;
	}

	gl_FragColor = clamp(vResult, 0.0, 1.0);
}