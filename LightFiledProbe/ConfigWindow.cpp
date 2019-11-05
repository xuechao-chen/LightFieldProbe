#include "ConfigWindow.h"

shared_ptr<CConfigWindow> CConfigWindow::create(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus)
{
	return shared_ptr<CConfigWindow>(new CConfigWindow(vApp, vGIRenderer, vProbeStatus));
}

CConfigWindow::CConfigWindow(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus) :
	GuiWindow("GI Config", vApp->debugWindow->theme(), Rect2D::xywh(0.0f, 100.0f, 100.0f, 150.0f), GuiTheme::TOOL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE),
	m_pApp(vApp),
	m_pGIRenderer(vGIRenderer),
	m_pProbeStatus(vProbeStatus)
{
	__makeGUI();
}

void CConfigWindow::__makeGUI()
{
	GuiTabPane* pConfigTabPane = pane()->addTabPane();
	GuiPane* pLightingPane = pConfigTabPane->addTab("Lighting");
	GuiPane* pProbePane = pConfigTabPane->addTab("Probe");
	
	pLightingPane->addCheckBox("Direct", &m_pGIRenderer->m_Settings.Direct);
	pLightingPane->addCheckBox("Indirect Diffuse", &m_pGIRenderer->m_Settings.IndirectDiffuse);
	pLightingPane->addCheckBox("Indirect Glossy", &m_pGIRenderer->m_Settings.IndirectGlossy);
	
	pProbePane->addCheckBox("Display Probe", &m_pGIRenderer->m_Settings.DisplayProbe);

	GuiPane* pPaddingPane = pProbePane->addPane("Padding");
	pPaddingPane->beginRow(); {
		pPaddingPane->addNumberBox(" X", &m_pProbeStatus->PositivePadding.x, "", GuiTheme::LOG_SLIDER, 0.1f, 0.5f);
		pPaddingPane->addNumberBox("-X", &m_pProbeStatus->NegativePadding.x, "", GuiTheme::LOG_SLIDER, 0.1f, 0.5f);
	} pPaddingPane->endRow();
	pPaddingPane->beginRow(); {
		pPaddingPane->addNumberBox(" Y", &m_pProbeStatus->PositivePadding.y, "", GuiTheme::LOG_SLIDER, 0.1f, 0.5f);
		pPaddingPane->addNumberBox("-Y", &m_pProbeStatus->NegativePadding.y, "", GuiTheme::LOG_SLIDER, 0.1f, 0.5f);
	} pPaddingPane->endRow();
	pPaddingPane->beginRow(); {
		pPaddingPane->addNumberBox(" Z", &m_pProbeStatus->PositivePadding.z, "", GuiTheme::LOG_SLIDER, 0.1f, 0.5f);
		pPaddingPane->addNumberBox("-Z", &m_pProbeStatus->NegativePadding.z, "", GuiTheme::LOG_SLIDER, 0.1f, 0.5f);
	} pPaddingPane->endRow();

	GuiPane* pNumPane = pProbePane->addPane("Count");
	pNumPane->beginRow(); {
		pNumPane->addNumberBox("X", &m_pProbeStatus->NewProbeCounts.x, "", GuiTheme::NO_SLIDER);
		pNumPane->addNumberBox("Y", &m_pProbeStatus->NewProbeCounts.y, "", GuiTheme::NO_SLIDER);
		pNumPane->addNumberBox("Z", &m_pProbeStatus->NewProbeCounts.z, "", GuiTheme::NO_SLIDER);
	} pNumPane->endRow();
	
	pProbePane->addButton("PreCompute", [this] {__onPrecompute(); });
}

void CConfigWindow::__onPrecompute()
{
	m_pProbeStatus->ProbeCounts = m_pProbeStatus->NewProbeCounts;
	m_pProbeStatus->updateStatus();
	m_pApp->precompute();
}
