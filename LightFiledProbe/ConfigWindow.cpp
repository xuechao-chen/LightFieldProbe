#include "ConfigWindow.h"
#include <fstream>
#include "G3D-app/FileDialog.h"

const String CConfigWindow::m_FileSuffix = ".cfg";
const String CConfigWindow::m_ProbeSuffix = ".probe";
const String CConfigWindow::m_CameraSuffix = ".cam";

CConfigWindow::CConfigWindow(App* vApp) :
	GuiWindow("GI Config(TAB)", vApp->debugWindow->theme(), Rect2D::xywh(0, 0, 100.0f, 150.0f), GuiTheme::TOOL_WINDOW_STYLE, GuiWindow::HIDE_ON_CLOSE),
	m_pApp(vApp)
{
	m_ProbeStatus.ProbeCounts = Vector3int32(2, 2, 2);
	m_ProbeStatus.NegativePadding = Vector3(0.1f, 0.1f, 0.1f);
	m_ProbeStatus.PositivePadding = Vector3(0.1f, 0.1f, 0.1f);

	m_hotKey = GKey::TAB;
	m_hotKeyMod = GKeyMod::NONE;
	__makeGUI();
}

void CConfigWindow::__makeGUI()
{
	GuiTabPane* pConfigTabPane = pane()->addTabPane(); {
		GuiPane* pLightingPane = pConfigTabPane->addTab("Lighting"); {
			__makeLightingPane(pLightingPane);
		} pLightingPane->pack();

		GuiPane* pProbePane = pConfigTabPane->addTab("Probe"); {
			__makeProbePane(pProbePane);
		} pProbePane->pack();

		GuiPane* pCameraPane = pConfigTabPane->addTab("Camera"); {
			__makeCameraPane(pCameraPane);
		} pCameraPane->pack();

	} pConfigTabPane->pack();
}

void CConfigWindow::__makeLightingPane(GuiPane* vLightingPane)
{
	vLightingPane->addCheckBox("Direct", &(m_pApp->m_pGIRenderer->m_Settings.Direct));
	vLightingPane->addCheckBox("Indirect Diffuse", &(m_pApp->m_pGIRenderer->m_Settings.IndirectDiffuse));
	vLightingPane->addCheckBox("Indirect Glossy", &(m_pApp->m_pGIRenderer->m_Settings.IndirectGlossy));
}

void CConfigWindow::__makeProbePane(GuiPane* vProbePane)
{
	vProbePane->beginRow(); {
		vProbePane->addCheckBox("Display Probe", &(m_pApp->m_pGIRenderer->m_Settings.DisplayProbe));
		vProbePane->addButton("PreCompute", [this] { m_pApp->precompute(); });
	} vProbePane->endRow();

	GuiPane* pPaddingPane = vProbePane->addPane("Padding"); {
		float MinPadding = -0.1f;
		float MaxPadding = 1.1f;
		float PaddingStep = 0.01f;
		pPaddingPane->beginRow(); {
			auto pPositiveXSlider = pPaddingPane->addNumberBox(" X", &m_ProbeStatus.PositivePadding.x, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
			pPositiveXSlider->setCaptionWidth(10.0f);
			auto pNegativeXSlider = pPaddingPane->addNumberBox("-X", &m_ProbeStatus.NegativePadding.x, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
			pNegativeXSlider->setCaptionWidth(10.0f);
		} pPaddingPane->endRow();
		pPaddingPane->beginRow(); {
			auto pPositiveYSlider = pPaddingPane->addNumberBox(" Y", &m_ProbeStatus.PositivePadding.y, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
			pPositiveYSlider->setCaptionWidth(10.0f);
			auto pNegativeYSlider = pPaddingPane->addNumberBox("-Y", &m_ProbeStatus.NegativePadding.y, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
			pNegativeYSlider->setCaptionWidth(10.0f);
		} pPaddingPane->endRow();
		pPaddingPane->beginRow(); {
			auto pPositiveZSlider = pPaddingPane->addNumberBox(" Z", &m_ProbeStatus.PositivePadding.z, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
			pPositiveZSlider->setCaptionWidth(10.0f);
			auto pNegativeZSlider = pPaddingPane->addNumberBox("-Z", &m_ProbeStatus.NegativePadding.z, "", GuiTheme::LOG_SLIDER, MinPadding, MaxPadding, PaddingStep);
			pNegativeZSlider->setCaptionWidth(10.0f);
		} pPaddingPane->endRow();
	} pPaddingPane->pack();

	GuiPane* pNumPane = vProbePane->addPane("Count"); {
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
		AddRowButton("X", m_ProbeStatus.ProbeCounts.x);
		AddRowButton("Y", m_ProbeStatus.ProbeCounts.y);
		AddRowButton("Z", m_ProbeStatus.ProbeCounts.z);

		pNumPane->beginRow(); {
			auto pXNumberBox = pNumPane->addNumberBox("X", &m_ProbeStatus.ProbeCounts.x, "", GuiTheme::NO_SLIDER);
			pXNumberBox->setCaptionWidth(10.0f); pXNumberBox->setWidth(150.0f);
			auto pYNumberBox = pNumPane->addNumberBox("Y", &m_ProbeStatus.ProbeCounts.y, "", GuiTheme::NO_SLIDER);
			pYNumberBox->setCaptionWidth(10.0f); pYNumberBox->setWidth(150.0f);
			auto pZNumberBox = pNumPane->addNumberBox("Z", &m_ProbeStatus.ProbeCounts.z, "", GuiTheme::NO_SLIDER);
			pZNumberBox->setCaptionWidth(10.0f); pZNumberBox->setWidth(150.0f);
		} pNumPane->endRow();
	} pNumPane->pack();

	GuiPane* pFilePane = vProbePane->addPane("File"); {
		pFilePane->beginRow(); {
			pFilePane->addButton("Load from default", [this]() {  __operateProbeFile(false, true); });
			pFilePane->addButton("Save to default ", [this]() { __operateProbeFile(true, true); });
			pFilePane->addButton("Load from ...", [this]() { __operateProbeFile(false, false); });
			pFilePane->addButton("Save to ...", [this]() { __operateProbeFile(true, false); });
		} pFilePane->endRow();
	} pFilePane->pack();
}

void CConfigWindow::__makeCameraPane(GuiPane* vCameraPane)
{
	GuiFrameBox* pCameraFrame = vCameraPane->addFrameBox(Pointer<CFrame>(this, &CConfigWindow::fetchCameraFrame, &CConfigWindow::setCameraFrame));
	pCameraFrame->setWidth(250.0f);
	vCameraPane->beginRow(); {
		vCameraPane->addButton("Load from default", [this]() { __operateCameraFile(false, true); });
		vCameraPane->addButton("Store to default", [this]() { __operateCameraFile(true, true); });
		vCameraPane->addButton("Load from ...", [this]() { __operateCameraFile(false, false); });
		vCameraPane->addButton("Store to ...", [this]() { __operateCameraFile(true, false); });
	} vCameraPane->endRow();
}

void CConfigWindow::__operateProbeFile(bool vIsOutput, bool vIsDefaultMode)
{
	String FilePath;
	if (vIsDefaultMode) FilePath = m_pApp->scene()->name() + m_ProbeSuffix + m_FileSuffix;
	else FileDialog::getFilename(FilePath, m_FileSuffix, vIsOutput);

	if (vIsOutput)
	{
		std::ofstream File(FilePath.c_str());
		File << m_ProbeStatus;
	}
	else
	{
		std::ifstream File(FilePath.c_str());
		File >> m_ProbeStatus;
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
