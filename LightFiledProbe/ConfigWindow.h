#pragma once
#include "App.h"
#include <string>

class CConfigWindow : public GuiWindow
{	
public:
	static shared_ptr<CConfigWindow> create(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus);
	
protected:
	CConfigWindow(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus);

	//bool onEvent(const GEvent& event) override;
	
private:
	void __makeGUI();
	void __onPrecompute();
	void __operateProbeFile(bool vIsOutput, bool vIsDefaultMode);
	void __operateCameraFile(bool vIsOutput, bool vIsDefaultMode);
	CFrame fetchCameraFrame() const { return m_pApp->m_activeCamera->frame(); }
	void setCameraFrame(const CFrame& f) { m_pApp->m_cameraManipulator->setFrame(f); }

	void __save2File(const std::string& vFilePath);
	void __loadFromFile(const std::string& vFilePath);

	App *m_pApp = nullptr;
	GKey                         m_hotKey;
	GKeyMod                      m_hotKeyMod;
	
	shared_ptr<SProbeStatus>     m_pProbeStatus;
	shared_ptr<CProbeGIRenderer> m_pGIRenderer;

	static const String m_FileSuffix;
	static const String m_ProbeSuffix;
	static const String m_CameraSuffix;
};