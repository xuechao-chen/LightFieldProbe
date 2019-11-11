#define M_PI     3.14159265358979323846
#define M_PI2    6.28318530717958647692
#define M_INV_PI 0.3183098861837906715

float rand_next(inout uint s)
{
	uint LCG_A = 1664525u;
	uint LCG_C = 1013904223u;
	s = (LCG_A * s + LCG_C);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

vec3 getPerpendicularStark(vec3 u)
{
	vec3 a = abs(u);
	uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
	uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, vec3(xm, ym, zm));
}

vec3 getGGXMicrofacet(vec2 u, vec3 N, float roughness)
{
	float a2 = roughness * roughness;

	float phi = M_PI2 * u.x;
	float cosTheta = sqrt(max(0, (1 - u.y)) / (1 + (a2 * a2 - 1) * u.y));
	float sinTheta = sqrt(max(0, 1 - cosTheta * cosTheta));

	// Tangent space H
	vec3 tH;
	tH.x = sinTheta * cos(phi);
	tH.y = sinTheta * sin(phi);
	tH.z = cosTheta;

	vec3 T = getPerpendicularStark(N);
	vec3 B = normalize(cross(N, T));

	// World space H
	return normalize(T * tH.x + B * tH.y + N * tH.z);
}

float evalGGX(float roughness, float NdotH)
{
	float a2 = roughness * roughness;
	float d = ((NdotH * a2 - NdotH) * NdotH + 1);
	return a2 / (d * d);
}

float evalSmithGGX(float NdotL, float NdotV, float roughness)
{
	// Optimized version of Smith, already taking into account the division by (4 * NdotV)
	float a2 = roughness * roughness;
	// `NdotV *` and `NdotL *` are inversed. It's not a mistake.
	float ggxv = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
	float ggxl = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
	return 0.5f / (ggxv + ggxl);
}

vec3 fresnelSchlick(vec3 f0, vec3 f90, float u)
{
	return f0 + (f90 - f0) * pow(1 - u, 5);
}