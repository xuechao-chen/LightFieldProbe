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

private:
	void __makeGUI();
	void __enableEmissiveLight(bool vEnable);
	void __precomputeLightFieldSurface(SLightFieldSurface& vioLightFieldSurface);
	void __renderLightFieldProbe(uint32 vProbeIndex, int vResolution, SLightFieldCubemap& voLightFieldCubemap);

	SLightFieldSurface   __initLightFieldSurface();
	shared_ptr<Texture>  __createSphereSampler(int vDegreeSize = 64);
	std::vector<Vector3> __placeProbe(const SLightFieldSurface& vLightFieldSurface);
};