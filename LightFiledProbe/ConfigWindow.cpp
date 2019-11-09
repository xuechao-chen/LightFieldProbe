#include "ConfigWindow.h"
#include <fstream>
#include "G3D-app/FileDialog.h"

const String CConfigWindow::m_FileSuffix = ".cfg";
const String CConfigWindow::m_ProbeSuffix = ".probe";
const String CConfigWindow::m_CameraSuffix = ".cam";

shared_ptr<CConfigWindow> CConfigWindow::create(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus)
{
	return shared_ptr<CConfigWindow>(new CConfigWindow(vApp, vGIRenderer, vProbeStatus));
}

CConfigWindow::CConfigWindow(App* vApp, const shared_ptr<CProbeGIRenderer>& vGIRenderer, const shared_ptr<SProbeStatus>& vProbeStatus) :
	GuiWindow("GI Config(TAB)", vApp->debugWindow->theme(), Rect2D::xywh(0, 0, 100.0f, 150.0f), GuiTheme::TOOL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE),
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
	GuiTabPane* pConfigTabPane = pane()->addTabPane(); {
		GuiPane* pLightingPane = pConfigTabPane->addTab("Lighting"); {
			pLightingPane->addCheckBox("Direct", &m_pGIRenderer->m_Settings.Direct);
			pLightingPane->addCheckBox("Indirect Diffuse", &m_pGIRenderer->m_Settings.IndirectDiffuse);
			pLightingPane->addCheckBox("Indirect Glossy", &m_pGIRenderer->m_Settings.IndirectGlossy);
		} pLightingPane->pack();
		
		GuiPane* pProbePane = pConfigTabPane->addTab("Probe"); {
			pProbePane->beginRow(); {
				pProbePane->addCheckBox("Display Probe", &m_pGIRenderer->m_Settings.DisplayProbe);
				pProbePane->addButton("PreCompute", [this] {__onPrecompute(); });
			} pProbePane->endRow();

			GuiPane* pPaddingPane = pProbePane->addPane("Padding"); {
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
			} pPaddingPane->pack();

			GuiPane* pNumPane = pProbePane->addPane("Count"); {
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
					pXNumberBox->setCaptionWidth(10.0f); pXNumberBox->setWidth(150.0f);
					auto pYNumberBox = pNumPane->addNumberBox("Y", &m_pProbeStatus->NewProbeCounts.y, "", GuiTheme::NO_SLIDER);
					pYNumberBox->setCaptionWidth(10.0f); pYNumberBox->setWidth(150.0f);
					auto pZNumberBox = pNumPane->addNumberBox("Z", &m_pProbeStatus->NewProbeCounts.z, "", GuiTheme::NO_SLIDER);
					pZNumberBox->setCaptionWidth(10.0f); pZNumberBox->setWidth(150.0f);
				} pNumPane->endRow();
			} pNumPane->pack();

			GuiPane* pFilePane = pProbePane->addPane("File"); {
				pFilePane->beginRow(); {
					pFilePane->addButton("Load from default", [this]() {  __operateProbeFile(false, true); });
					pFilePane->addButton("Save to default ", [this]() { __operateProbeFile(true, true); });
					pFilePane->addButton("Load from ...", [this]() { __operateProbeFile(false, false); });
					pFilePane->addButton("Save to ...", [this]() { __operateProbeFile(true, false); });
				} pFilePane->endRow();
			} pFilePane->pack();
		} pProbePane->pack();

		GuiPane* pCameraPane = pConfigTabPane->addTab("Camera"); {
			GuiFrameBox* pCameraFrame = pCameraPane->addFrameBox(Pointer<CFrame>(this, &CConfigWindow::fetchCameraFrame, &CConfigWindow::setCameraFrame));
			pCameraFrame->setWidth(250.0f);
			pCameraPane->beginRow(); {
				pCameraPane->addButton("Load from default", [this]() { __operateCameraFile(false, true); });
				pCameraPane->addButton("Store to default", [this]() { __operateCameraFile(true, true); });
				pCameraPane->addButton("Load from ...", [this]() { __operateCameraFile(false, false); });
				pCameraPane->addButton("Store to ...", [this]() { __operateCameraFile(true, false); });
			} pCameraPane->endRow();
		} pCameraPane->pack();
		
	} pConfigTabPane->pack();
}

void CConfigWindow::__onPrecompute()
{
	m_pProbeStatus->updateStatus();
	m_pApp->precompute();
}

void CConfigWindow::__operateProbeFile(bool vIsOutput, bool vIsDefaultMode)
{
	String FilePath;
	if (vIsDefaultMode) FilePath = m_pApp->scene()->name() + m_ProbeSuffix + m_FileSuffix;
	else FileDialog::getFilename(FilePath, m_FileSuffix, vIsOutput);

	if (vIsOutput)
	{
		std::ofstream File(FilePath.c_str());
		File << *m_pProbeStatus;
	}
	else
	{
		std::ifstream File(FilePath.c_str());
		File >> *m_pProbeStatus;
	}
}

void CConfigWindow::__operateCameraFile(bool vIsOutput, bool vIsDefaultMode)
{
	String FilePath;
	if (vIsDefaultMode) FilePath = m_pApp->scene()->name() + m_CameraSuffix + m_FileSuffix;
	else FileDialog::getFilename(FilePath, m_FileSuffix, vIsOutput);

	Matrix3 Rotation;
	Point3 Translation;
	
	if (vIsOutput)
	{
		Rotation = m_pApp->m_activeCamera->frame().rotation;
		Translation = m_pApp->m_activeCamera->frame().translation;
		std::ofstream File(FilePath.c_str());
		for (int i = 0; i < 3; i++)
			for (int k = 0; k < 3; k++)
				File << Rotation[i][k] << " ";
		File << std::endl;
		File << Translation[0] << " " << Translation[1] << " " << Translation[2] << " " << std::endl;
	}
	else
	{		
		std::ifstream File(FilePath.c_str());
		for (int i = 0; i < 3; i++)
			for (int k = 0; k < 3; k++)
				File >> Rotation[i][k];
		File >> Translation[0] >> Translation[1] >> Translation[2];
		m_pApp->m_cameraManipulator->setFrame(CFrame(Rotation, Translation));
	}
}

void CConfigWindow::__save2File(const std::string& vFilePath)
{
	std::ofstream OutputFile(vFilePath);
	OutputFile << (*m_pProbeStatus);
}

void CConfigWindow::__loadFromFile(const std::string& vFilePath)
{
	std::ifstream InputFile(vFilePath);
	InputFile >> (*m_pProbeStatus);
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
