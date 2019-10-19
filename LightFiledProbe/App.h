#pragma once
#include <G3D/G3D.h>

struct SLightFieldSurface
{
	shared_ptr<Texture>     RadianceProbeGrid;
	shared_ptr<Texture>     IrradianceProbeGrid;
	shared_ptr<Texture>		DistanceProbeGrid;
	shared_ptr<Texture>		MeanDistProbeGrid;

	Vector3int32            ProbeCounts;
	Point3                  ProbeStartPosition;
	Vector3                 ProbeSteps;
};

class App : public GApp
{
	shared_ptr<Framebuffer> m_ProbeGBuffer;

	SLightFieldSurface   m_LightFieldSurface;
	std::vector<Vector3> m_ProbePositionSet;

	bool m_IsPrecomputed = false;
public:
	App(const GApp::Settings& vSettings) : GApp(vSettings) {}

	virtual void onInit() override;
	virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) override;

private:
	void __makeGUI();
	void __precomputeLightFieldSurface(SLightFieldSurface& vioLightFieldSurface);
	shared_ptr<Texture>  __createSphereSampler(int vDegreeSize = 64);
	SLightFieldSurface   __initLightFieldSurface();
	std::vector<Vector3> __placeProbe(const SLightFieldSurface& vLightFieldSurface);
	void __renderLightFieldProbe(uint32 vProbeIndex, shared_ptr<Texture> voRadianceCubemap, shared_ptr<Texture> voDistanceCubemap);
};