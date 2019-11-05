#pragma once
#include "App.h"

class CConfigWindow : public GuiWindow
{
public:
	static shared_ptr<CConfigWindow> create(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus);
	
protected:
	CConfigWindow(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus);

private:
	void __makeGUI();
	void __onPrecompute();

	App *m_pApp = nullptr;
	// TODO: Hot key 
	
	shared_ptr<SProbeStatus>     m_pProbeStatus;
	shared_ptr<CProbeGIRenderer> m_pGIRenderer;
};