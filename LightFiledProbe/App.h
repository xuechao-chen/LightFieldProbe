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
	void __specifyGBufferEncoding();
	void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) override;

private:
	void __makeGUI();
	void __enableEmissiveLight(bool vEnable);
	void __precomputeLightFieldSurface(SLightFieldSurface& vioLightFieldSurface);
	void __generateLowResOctmap(std::shared_ptr<Framebuffer>& vLightFieldFramebuffer, SLightFieldCubemap& vLightFieldCubemap, shared_ptr<Texture>& vSphereSamplerTexture);
	void __generateHighResOctmap(std::shared_ptr<Framebuffer>& vLightFieldFramebuffer, SLightFieldCubemap& vLightFieldCubemap);
	void __renderLightFieldProbe2Cubemap(uint32 vProbeIndex, int vResolution, SLightFieldCubemap& voLightFieldCubemap);

	SLightFieldSurface   __initLightFieldSurface();
	shared_ptr<Texture>  __createSphereSampler(int vDegreeSize = 64);
	std::vector<Vector3> __placeProbe(const SLightFieldSurface& vLightFieldSurface);
};