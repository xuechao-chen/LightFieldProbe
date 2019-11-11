#pragma once
#include <G3D/G3D.h>
#include <iostream>
#include <sstream>

struct SLightFieldSurfaceMetaData
{
	Vector2int32			SphereSamplerSize;
	Vector3int32            ProbeCounts;
	Vector3                 ProbeStartPos;
	Vector3                 ProbeSteps;

	int OctResolution;
	int ProbeCubemapResolution;
	int LightCubemapResolution;
	
	int ProbeNum() const { return ProbeCounts.x * ProbeCounts.y * ProbeCounts.z; }

	Vector3 ProbeIndexToPosition(int vIndex) const
	{
		int z = vIndex / (ProbeCounts.x * ProbeCounts.y);
		vIndex %= (ProbeCounts.x * ProbeCounts.y);
		int y = vIndex / ProbeCounts.x;
		int x = vIndex % ProbeCounts.x; // NOTE: dangerous

		return ProbeStartPos + Vector3(x, y, z) * ProbeSteps;
	}
};

struct SLightFieldSurface
{
	shared_ptr<Texture>     IrradianceProbeGrid;
	shared_ptr<Texture>		MeanDistProbeGrid;
};

struct SLightFieldCubemap
{
	SLightFieldCubemap(int vResolution) : SLightFieldCubemap(vResolution, ImageFormat::RGB32F()) {}

	SLightFieldCubemap(int vResolution, Texture::Encoding vImageFormat)
	{
		PositionCubemap = Texture::createEmpty("DistanceCubemap", vResolution, vResolution, vImageFormat, Texture::DIM_CUBE_MAP, true);
	}

	shared_ptr<Texture> PositionCubemap;
};

struct SProbeStatus
{
	Vector3int32            ProbeCounts;
	Vector3                 ProbeStartPos;
	Vector3                 ProbeSteps;

	Vector3                 PositivePadding;
	Vector3                 NegativePadding;
	Vector3                 BoundingBoxLow;
	Vector3                 BoundingBoxHigh;

	void updateStatus()
	{
		Vector3 BoundingBoxRange = BoundingBoxHigh - BoundingBoxLow;
		ProbeStartPos = BoundingBoxLow + BoundingBoxRange * NegativePadding;
		Vector3 ProbeEndPos = BoundingBoxHigh - BoundingBoxRange * PositivePadding;
		ProbeSteps = (ProbeEndPos - ProbeStartPos) / (ProbeCounts - Vector3int32(1, 1, 1));

		if (ProbeCounts.x == 1) { ProbeSteps.x = BoundingBoxRange.x * 0.5f; ProbeStartPos.x = (ProbeStartPos.x + ProbeEndPos.x) * 0.5f; }
		if (ProbeCounts.y == 1) { ProbeSteps.y = BoundingBoxRange.y * 0.5f; ProbeStartPos.y = (ProbeStartPos.y + ProbeEndPos.y) * 0.5f; }
		if (ProbeCounts.z == 1) { ProbeSteps.z = BoundingBoxRange.z * 0.5f; ProbeStartPos.z = (ProbeStartPos.z + ProbeEndPos.z) * 0.5f; }
	}
};

inline std::ostream& operator<<(std::ostream& vOutput, const SProbeStatus& vStatus)
{
	auto outputFormattedVec3 = [&](const Vector3& vVec3)
	{
		vOutput << vVec3.x << " " << vVec3.y << " " << vVec3.z << std::endl;
	};
	outputFormattedVec3(vStatus.ProbeCounts);
	outputFormattedVec3(vStatus.ProbeStartPos);
	outputFormattedVec3(vStatus.ProbeSteps);
	outputFormattedVec3(vStatus.PositivePadding);
	outputFormattedVec3(vStatus.NegativePadding);
	outputFormattedVec3(vStatus.BoundingBoxLow);
	outputFormattedVec3(vStatus.BoundingBoxHigh);

	return vOutput;
}

inline std::istream& operator>>(std::istream& vInput, SProbeStatus& voStatus)
{
	auto inputFormattedVec3 = [&](Vector3& vVec3)
	{
		vInput >> vVec3.x >> vVec3.y >> vVec3.z;
	};
	auto inputFormattedVec3I = [&](Vector3int32& vVec3)
	{
		vInput >> vVec3.x >> vVec3.y >> vVec3.z;
	};
	inputFormattedVec3I(voStatus.ProbeCounts);
	inputFormattedVec3(voStatus.ProbeStartPos);
	inputFormattedVec3(voStatus.ProbeSteps);
	inputFormattedVec3(voStatus.PositivePadding);
	inputFormattedVec3(voStatus.NegativePadding);
	inputFormattedVec3(voStatus.BoundingBoxLow);
	inputFormattedVec3(voStatus.BoundingBoxHigh);

	return vInput;
}
