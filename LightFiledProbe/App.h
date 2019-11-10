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
	
	shared_ptr<Renderer> m_pDefaultRenderer;
	shared_ptr<CProbeGIRenderer> m_pGIRenderer;

	shared_ptr<SLightFieldSurface> m_pLightFieldSurface;
	shared_ptr<SLightFieldSurfaceMetaData> m_pLightFieldSurfaceMetaData;
	shared_ptr<CLightFieldSurfaceGenerator> m_pLightFieldSurfaceGenerator;

public:
	App(const GApp::Settings& vSettings) : GApp(vSettings) {}
	~App() { m_pConfigWindow.reset(); }

	void precompute();
	
protected:
	virtual void onInit() override;
	virtual bool onEvent(const GEvent& event) override;
	virtual void onAfterLoadScene(const Any& any, const String& sceneName) override;
	virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>& allSurfaces) override;

private:
	void __makeGUI();
	void __refreshProbes(float vProbeRadius = -1.0f);
	void __enableEmissiveLight(bool vEnable);

	shared_ptr<SLightFieldSurfaceMetaData> __initLightFieldSurfaceMetaData();
};