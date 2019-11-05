#pragma once
#include <G3D/G3D.h>

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
		Vector3 BoundingBoxRange = BoundingBoxHeight - BoundingBoxLow;
		ProbeStartPos = BoundingBoxLow + BoundingBoxRange * NegativePadding;
		Vector3 ProbeEndPos = BoundingBoxHeight - BoundingBoxRange * PositivePadding;
		ProbeSteps = (ProbeEndPos - ProbeStartPos) / (ProbeCounts - Vector3int32(1, 1, 1));

		if (ProbeCounts.x == 1) { ProbeSteps.x = BoundingBoxRange.x * 0.5; ProbeStartPos.x = (ProbeStartPos.x + ProbeEndPos.x) * 0.5; }
		if (ProbeCounts.y == 1) { ProbeSteps.y = BoundingBoxRange.y * 0.5; ProbeStartPos.y = (ProbeStartPos.y + ProbeEndPos.y) * 0.5; }
		if (ProbeCounts.z == 1) { ProbeSteps.z = BoundingBoxRange.z * 0.5; ProbeStartPos.z = (ProbeStartPos.z + ProbeEndPos.z) * 0.5; }
	}
};

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