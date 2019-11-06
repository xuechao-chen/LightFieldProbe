#include "ConfigWindow.h"

shared_ptr<CConfigWindow> CConfigWindow::create(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus)
{
	return shared_ptr<CConfigWindow>(new CConfigWindow(vApp, vGIRenderer, vProbeStatus));
}

CConfigWindow::CConfigWindow(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus) :
	GuiWindow("GI Config", vApp->debugWindow->theme(), Rect2D::xywh(0, 0, 100.0f, 150.0f), GuiTheme::TOOL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE),
	m_pApp(vApp),
	m_pGIRenderer(vGIRenderer),
	m_pProbeStatus(vProbeStatus)
{
	m_hotKey = GKey::TAB;
	m_hotKeyMod = GKeyMod::NONE;
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

	pProbePane->beginRow(); {
		pProbePane->addCheckBox("Display Probe", &m_pGIRenderer->m_Settings.DisplayProbe);
		pProbePane->addButton("PreCompute", [this] {__onPrecompute(); });
	}pProbePane->endRow();
	
	GuiPane* pPaddingPane = pProbePane->addPane("Padding");
	float MinPadding = -0.1f;
	float MaxPadding = 1.1f;
	float PaddingStep = 0.01f;
	pPaddingPane->beginRow(); {
		auto pPositiveXSlider = pPaddingPane->addNumberBox(" X", &m_pProbeStatus->PositivePadding.x, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
		pPositiveXSlider->setCaptionWidth(10.0f);
		auto pNegativeXSlider = pPaddingPane->addNumberBox("-X", &m_pProbeStatus->NegativePadding.x, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
		pNegativeXSlider->setCaptionWidth(10.0f);
	} pPaddingPane->endRow();
	pPaddingPane->beginRow(); {
		auto pPositiveYSlider = pPaddingPane->addNumberBox(" Y", &m_pProbeStatus->PositivePadding.y, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
		pPositiveYSlider->setCaptionWidth(10.0f);
		auto pNegativeYSlider = pPaddingPane->addNumberBox("-Y", &m_pProbeStatus->NegativePadding.y, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
		pNegativeYSlider->setCaptionWidth(10.0f);
	} pPaddingPane->endRow();
	pPaddingPane->beginRow(); {
		auto pPositiveZSlider = pPaddingPane->addNumberBox(" Z", &m_pProbeStatus->PositivePadding.z, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
		pPositiveZSlider->setCaptionWidth(10.0f);
		auto pNegativeZSlider = pPaddingPane->addNumberBox("-Z", &m_pProbeStatus->NegativePadding.z, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
		pNegativeZSlider->setCaptionWidth(10.0f);
	} pPaddingPane->endRow();
	pPaddingPane->pack();
	
	GuiPane* pNumPane = pProbePane->addPane("Count");
	auto AddRowButton = [&](const GuiText& vText, int32_t& vBindingValue)
	{
		pNumPane->beginRow(); {
			pNumPane->addLabel(vText)->setWidth(10.0f);
			pNumPane->addButton(" 1", [&] { vBindingValue = 1; });
			pNumPane->addButton(" 2", [&] { vBindingValue = 2; });
			pNumPane->addButton(" 4", [&] { vBindingValue = 4; });
			pNumPane->addButton(" 8", [&] { vBindingValue = 8; });
		} pNumPane->endRow();
	};
	AddRowButton("X", m_pProbeStatus->NewProbeCounts.x);
	AddRowButton("Y", m_pProbeStatus->NewProbeCounts.y);
	AddRowButton("Z", m_pProbeStatus->NewProbeCounts.z);
	
	pNumPane->beginRow(); {
		auto pXNumberBox = pNumPane->addNumberBox("X", &m_pProbeStatus->NewProbeCounts.x, "", GuiTheme::NO_SLIDER);
		pXNumberBox->setCaptionWidth(10.0f); pXNumberBox->setWidth(100.0f);
		auto pYNumberBox = pNumPane->addNumberBox("Y", &m_pProbeStatus->NewProbeCounts.y, "", GuiTheme::NO_SLIDER);
		pYNumberBox->setCaptionWidth(10.0f); pYNumberBox->setWidth(100.0f);
		auto pZNumberBox = pNumPane->addNumberBox("Z", &m_pProbeStatus->NewProbeCounts.z, "", GuiTheme::NO_SLIDER);
		pZNumberBox->setCaptionWidth(10.0f); pZNumberBox->setWidth(100.0f);
	} pNumPane->endRow();
	pNumPane->pack();

	pLightingPane->pack();
	pProbePane->pack();

	pConfigTabPane->pack();
}

void CConfigWindow::__onPrecompute()
{
	m_pProbeStatus->updateStatus();
	m_pApp->precompute();
}

//bool CConfigWindow::onEvent(const GEvent& event)
//{
//	//if (enabled()) {
//	//	const bool hotKeyPressed = (event.type == GEventType::KEY_DOWN) && (event.key.keysym.sym == m_hotKey) && (event.key.keysym.mod == m_hotKeyMod);
//	//	if (hotKeyPressed) m_visible = !m_visible;
//	//}
//
//	if (event.type == GEventType::KEY_DOWN && event.key.keysym.sym == GKey::TAB)
//	{
//		m_visible = !m_visible;
//	}
//	
//	return GuiWindow::onEvent(event);
//}
