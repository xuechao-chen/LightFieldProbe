#pragma once
#include <G3D/G3D.h>

struct SLightFieldSurface
{
	shared_ptr<Texture>     RadianceProbeGrid;
	shared_ptr<Texture>     IrradianceProbeGrid;

	Vector3int32            ProbeCounts;
	Point3                  ProbeStartPosition;
	Vector3                 ProbeSteps;

};

class App : public GApp
{
	shared_ptr<Framebuffer> m_ProbeGBuffer;

	SLightFieldSurface   m_LightFiledSurface;
	std::vector<Vector3> m_ProbePositionSet;

	shared_ptr<Texture>  m_SphereSamplerTexture;

	bool m_IsPrecomputed = false;
public:
	App(const GApp::Settings& vSettings) : GApp(vSettings) {}

	virtual void onInit() override;
	virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) override;

private:
	void __makeGUI();
	void __precomputeLightFiledSurface(SLightFieldSurface& vioLightFiledSurface);
	shared_ptr<Texture>  __createSphereSampler(int vDegreeSize = 64);
	SLightFieldSurface   __initLightFiledSurface();
	std::vector<Vector3> __placeProbe(const SLightFieldSurface& vLightFiledSurface);
	void __renderLightFiledProbe(uint32 vProbeIndex, shared_ptr<Texture> voRadianceCubemap, shared_ptr<Texture> voDistanceCubemap);
};