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