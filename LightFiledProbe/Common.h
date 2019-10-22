#pragma once
#include <G3D/G3D.h>

struct SLightFieldSurface
{
	shared_ptr<Texture>     RadianceProbeGrid;
	shared_ptr<Texture>     IrradianceProbeGrid;
	shared_ptr<Texture>		DistanceProbeGrid;
	shared_ptr<Texture>		MeanDistProbeGrid;
	shared_ptr<Texture>     NormalProbeGrid;
	shared_ptr<Texture>     LowResolutionDistanceProbeGrid;

	Vector3int32            ProbeCounts;
	Point3                  ProbeStartPosition;
	Vector3                 ProbeSteps;
};

struct SLightFieldCubemap
{
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