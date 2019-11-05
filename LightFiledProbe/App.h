#pragma once
#include "Common.h"
#include "ProbeGIRenderer.h"

class CConfigWindow;

class App : public GApp
{
	friend class CConfigWindow;
	
	shared_ptr<CConfigWindow> m_pConfigWindow;
	
	shared_ptr<CProbeGIRenderer> m_pGIRenderer;
	shared_ptr<Renderer> m_DefaultRenderer;

	shared_ptr<SProbeStatus> m_pProbeStatus;
	shared_ptr<SLightFieldSurface> m_pLightFieldSurface;
	std::vector<Vector3> m_ProbePositionSet;

public:
	App(const GApp::Settings& vSettings) : GApp(vSettings) {}
	~App() { m_pConfigWindow.reset(); }
	
	virtual void onInit() override;
	virtual bool onEvent(const GEvent& event) override;

	void precompute();
	void __specifyGBufferEncoding();
	void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces) override;
	void onPreComputeGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces);
	
private:
	void __makeGUI();
	void __enableEmissiveLight(bool vEnable);
	void __precomputeLightFieldSurface(const shared_ptr<SLightFieldSurface>& vioLightFieldSurface);
	void __generateLowResOctmap(std::shared_ptr<Framebuffer>& vLightFieldFramebuffer, SLightFieldCubemap& vLightFieldCubemap, shared_ptr<Texture>& vSphereSamplerTexture);
	void __generateHighResOctmap(std::shared_ptr<Framebuffer>& vLightFieldFramebuffer, SLightFieldCubemap& vLightFieldCubemap);
	void __renderLightFieldProbe2Cubemap(uint32 vProbeIndex, int vResolution, SLightFieldCubemap& voLightFieldCubemap);

	shared_ptr<SLightFieldSurface>   __initLightFieldSurface();
	shared_ptr<SProbeStatus> __initProbeStatus();
	shared_ptr<Texture>  __createSphereSampler(int vDegreeSize = 64);
	std::vector<Vector3> __placeProbe();
};