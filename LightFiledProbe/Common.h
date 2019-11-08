#pragma once
#include <G3D/G3D.h>
#include <iostream>
#include <sstream>

struct SProbeStatus
{
	Vector3int32            ProbeCounts;
	Vector3int32            NewProbeCounts;
	Point3                  ProbeStartPos;
	Vector3                 ProbeSteps;

	Vector3                 PositivePadding;
	Vector3                 NegativePadding;
	Vector3                 BoundingBoxLow;
	Vector3                 BoundingBoxHeight;

	void updateStatus()
	{
		ProbeCounts = NewProbeCounts;
		Vector3 BoundingBoxRange = BoundingBoxHeight - BoundingBoxLow;
		ProbeStartPos = BoundingBoxLow + BoundingBoxRange * NegativePadding;
		Vector3 ProbeEndPos = BoundingBoxHeight - BoundingBoxRange * PositivePadding;
		ProbeSteps = (ProbeEndPos - ProbeStartPos) / (ProbeCounts - Vector3int32(1, 1, 1));

		if (ProbeCounts.x == 1) { ProbeSteps.x = BoundingBoxRange.x * 0.5; ProbeStartPos.x = (ProbeStartPos.x + ProbeEndPos.x) * 0.5; }
		if (ProbeCounts.y == 1) { ProbeSteps.y = BoundingBoxRange.y * 0.5; ProbeStartPos.y = (ProbeStartPos.y + ProbeEndPos.y) * 0.5; }
		if (ProbeCounts.z == 1) { ProbeSteps.z = BoundingBoxRange.z * 0.5; ProbeStartPos.z = (ProbeStartPos.z + ProbeEndPos.z) * 0.5; }
	}

	SProbeStatus getNewProbeStatus()
	{
		SProbeStatus NewProbeStatus = *this;
		NewProbeStatus.ProbeCounts = NewProbeStatus.NewProbeCounts;
		NewProbeStatus.updateStatus();
		return NewProbeStatus;
	}
};

inline std::ostream& operator<<(std::ostream& vOutput, const SProbeStatus& vStatus)
{
	auto outputFormattedVec3 = [&](const Vector3& vVec3)
	{
		vOutput << vVec3.x << " " << vVec3.y << " " << vVec3.z << std::endl;
	};
	outputFormattedVec3(vStatus.ProbeCounts);
	outputFormattedVec3(vStatus.NewProbeCounts);
	outputFormattedVec3(vStatus.ProbeStartPos);
	outputFormattedVec3(vStatus.ProbeSteps);
	outputFormattedVec3(vStatus.PositivePadding);
	outputFormattedVec3(vStatus.NegativePadding);
	outputFormattedVec3(vStatus.BoundingBoxLow);
	outputFormattedVec3(vStatus.BoundingBoxHeight);

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
	inputFormattedVec3I(voStatus.NewProbeCounts);
	inputFormattedVec3(voStatus.ProbeStartPos);
	inputFormattedVec3(voStatus.ProbeSteps);
	inputFormattedVec3(voStatus.PositivePadding);
	inputFormattedVec3(voStatus.NegativePadding);
	inputFormattedVec3(voStatus.BoundingBoxLow);
	inputFormattedVec3(voStatus.BoundingBoxHeight);

	return vInput;
}

struct SLightFieldSurface
{
	shared_ptr<Texture>     RadianceProbeGrid;
	shared_ptr<Texture>     IrradianceProbeGrid;
	shared_ptr<Texture>		DistanceProbeGrid;
	shared_ptr<Texture>		MeanDistProbeGrid;
	shared_ptr<Texture>     NormalProbeGrid;
	shared_ptr<Texture>     LowResolutionDistanceProbeGrid;
};

struct SLightFieldCubemap
{
	SLightFieldCubemap(int vResolution) : SLightFieldCubemap(vResolution, ImageFormat::RGB32F()) {}

	SLightFieldCubemap(int vResolution, Texture::Encoding vImageFormat)
	{
		RadianceCubemap = Texture::createEmpty("RadianceCubemap", vResolution, vResolution, vImageFormat, Texture::DIM_CUBE_MAP);
		DistanceCubemap = Texture::createEmpty("DistanceCubemap", vResolution, vResolution, vImageFormat, Texture::DIM_CUBE_MAP);
		NormalCubemap = Texture::createEmpty("NormalCubemap", vResolution, vResolution, vImageFormat, Texture::DIM_CUBE_MAP);
	}

	shared_ptr<Texture> RadianceCubemap;
	shared_ptr<Texture> DistanceCubemap;
	shared_ptr<Texture> NormalCubemap;
};