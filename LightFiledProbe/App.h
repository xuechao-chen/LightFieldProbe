#pragma once
#include "Common.h"
#include "ProbeGIRenderer.h"

class CConfigWindow;
class CLightFieldSurfaceGenerator;

class App : public GApp
{
	friend class CConfigWindow;
	friend class CLightFieldSurfaceGenerator;

	shared_ptr<CConfigWindow> m_pConfigWindow;
	
	shared_ptr<CProbeGIRenderer> m_pGIRenderer;
	shared_ptr<Renderer> m_pDefaultRenderer;

	shared_ptr<CLightFieldSurfaceGenerator> m_pLightFieldSurfaceGenerator;

	shared_ptr<SLightFieldSurfaceMetaData> m_pLightFieldSurfaceMetaData;
	shared_ptr<SLightFieldSurface> m_pLightFieldSurface;

public:
	App(const GApp::Settings& vSettings) : GApp(vSettings) {}
	~App() { m_pConfigWindow.reset(); }
	
	virtual void onInit() override;
	virtual bool onEvent(const GEvent& event) override;

	void precompute();
	void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>& allSurfaces) override;
	virtual void onAfterLoadScene(const Any& any, const String& sceneName) override;

private:
	void __makeGUI();
	void __refreshProbes(float vProbeRadius = -1.0f);
	void __enableEmissiveLight(bool vEnable);

	shared_ptr<SProbeStatus> __initProbeStatus();
	shared_ptr<SLightFieldSurfaceMetaData> __initLightFieldSurfaceMetaData();
};