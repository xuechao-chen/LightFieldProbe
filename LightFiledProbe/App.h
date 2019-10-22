#pragma once
#include "Common.h"
#include "ProbeGIRenderer.h"

class App : public GApp
{
	shared_ptr<CProbeGIRenderer> m_pGIRenderer;
	SLightFieldSurface   m_LightFieldSurface;
	std::vector<Vector3> m_ProbePositionSet;
public:
	App(const GApp::Settings& vSettings) : GApp(vSettings) {}

	virtual void onInit() override;
	//virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) override;

private:
	void __makeGUI();
	void __precomputeLightFieldSurface(SLightFieldSurface& vioLightFieldSurface);
	shared_ptr<Texture>  __createSphereSampler(int vDegreeSize = 64);
	SLightFieldSurface   __initLightFieldSurface();
	std::vector<Vector3> __placeProbe(const SLightFieldSurface& vLightFieldSurface);
	void __renderLightFieldProbe(uint32 vProbeIndex, int vResolution, SLightFieldSurface& voLightFieldSurface);
	void __enableEmissiveLight(bool vEnable);
};