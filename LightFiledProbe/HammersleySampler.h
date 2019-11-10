#pragma once
#include <vector>
#include <G3D/G3D.h>

class CHammersleySampler
{
public:
	std::vector<Vector3> sampleSphereUniformly(uint vSampleNum, bool vHemisphere = false)
	{
		std::vector<Vector3> SampleDirections;
		for (uint i = 0; i < vSampleNum; ++i)
		{
			auto UV = __hammersley2d(i, vSampleNum);
			Vector3 Direction = __mapUV2Sphere(UV.x, UV.y, vHemisphere);
			SampleDirections.push_back(Direction);
		}

		return SampleDirections;
	}

private:
	float __radicalInverse(uint vBits)
	{
		vBits = (vBits << 16u) | (vBits >> 16u);
		vBits = ((vBits & 0x55555555u) << 1u) | ((vBits & 0xAAAAAAAAu) >> 1u);
		vBits = ((vBits & 0x33333333u) << 2u) | ((vBits & 0xCCCCCCCCu) >> 2u);
		vBits = ((vBits & 0x0F0F0F0Fu) << 4u) | ((vBits & 0xF0F0F0F0u) >> 4u);
		vBits = ((vBits & 0x00FF00FFu) << 8u) | ((vBits & 0xFF00FF00u) >> 8u);
		return float(vBits) * 2.3283064365386963e-10; // / 0x100000000
	}

	Vector2 __hammersley2d(uint i, uint N) 
	{
		return Vector2(float(i) / float(N), __radicalInverse(i));
	}

	Vector3 __mapUV2Sphere(float u, float v, bool vHemisphere) 
	{
		float Phi = v * 2.0 * pi();
		float CosTheta = vHemisphere ? (1.0 - u) : (u * 2 - 1);
		float SinTheta = sqrt(1.0 - CosTheta * CosTheta);
		return Vector3(cos(Phi) * SinTheta, sin(Phi) * SinTheta, CosTheta);
	}
};