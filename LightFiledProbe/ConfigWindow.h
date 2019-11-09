#pragma once
#include "App.h"
#include <string>

class CConfigWindow : public GuiWindow
{	
	App *m_pApp = nullptr;
	GKey                         m_hotKey;
	GKeyMod                      m_hotKeyMod;

	SProbeStatus     m_ProbeStatus;
	shared_ptr<CProbeGIRenderer> m_pGIRenderer;

	static const String m_FileSuffix;
	static const String m_ProbeSuffix;
	static const String m_CameraSuffix;

public:
	CConfigWindow(App* vApp);
	const SProbeStatus& getProbeStatus() const { return m_ProbeStatus; }
	void setProbeStatus(const SProbeStatus& vStatus) { m_ProbeStatus = vStatus; }

	static shared_ptr<CConfigWindow> create(App* vApp) { return std::make_shared<CConfigWindow>(vApp); }

private:
	void __operateProbeFile(bool vIsOutput, bool vIsDefaultMode);
	void __operateCameraFile(bool vIsOutput, bool vIsDefaultMode);

	void __makeGUI();
	void __makeLightingPane(GuiPane* vLightPane);
	void __makeProbePane(GuiPane* vProbePane);
	void __makeCameraPane(GuiPane* vCameraPane);

	// TODO:
	CFrame fetchCameraFrame() const { return m_pApp->m_activeCamera->frame(); }
	void setCameraFrame(const CFrame& f) { m_pApp->m_cameraManipulator->setFrame(f); }
};