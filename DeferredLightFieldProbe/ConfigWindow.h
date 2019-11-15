#pragma once
#include "App.h"

class CConfigWindow : public GuiWindow
{	
	static const String m_FileSuffix;
	static const String m_ProbeSuffix;
	static const String m_CameraSuffix;

	App* m_pApp = nullptr;
	SProbeStatus m_ProbeStatus;
	int m_LodLevel = 1;

public:
	SProbeStatus& fetchProbeStatus() { return m_ProbeStatus; }
	void  setProbeStatus(const SProbeStatus& vStatus) { m_ProbeStatus = vStatus; }
	int getLodLevel() const { return m_LodLevel; }

	static shared_ptr<CConfigWindow> create(App* vApp) { return createShared<CConfigWindow>(vApp); }

protected:
	CConfigWindow(App* vApp);

private:
	void __operateProbeFile(bool vIsOutput, bool vIsDefaultMode);
	void __operateCameraFile(bool vIsOutput, bool vIsDefaultMode);

	void __makeGUI();
	void __makeLightingPane(GuiPane* vLightPane);
	void __makeProbePane(GuiPane* vProbePane);
	void __makeCameraPane(GuiPane* vCameraPane);
	void __makeModelPane(GuiPane* vModelPane);

	CFrame __fetchCameraFrame() const { return m_pApp->m_activeCamera->frame(); }
	void __setCameraFrame(const CFrame& f) { m_pApp->m_cameraManipulator->setFrame(f); }
};