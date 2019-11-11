#pragma once
#include "App.h"
#include "ConfigWindow.h"
#include "HammersleySampler.h"
#include "LightFieldSurfaceGenerator.h"

void App::onInit()
{
	std::vector<std::string> AllScenes = {
		"Simple Cornell Box",
		"Simple Cornell Box (No Little Boxes)",
		"Sponza (Glossy Area Lights)",
		"Sponza (Statue Glossy)"
	};

	GApp::onInit();
	setFrameDuration(1.0f / 60.f);

	m_pDefaultRenderer = m_renderer;
	m_pGIRenderer = dynamic_pointer_cast<CProbeGIRenderer>(CProbeGIRenderer::create());
	m_pGIRenderer->setDeferredShading(true);

	m_pConfigWindow = CConfigWindow::create(this);

	loadScene(AllScenes[0].c_str());

	m_pLightFieldSurfaceMetaData = __initLightFieldSurfaceMetaData();
	m_pLightFieldSurfaceGenerator = CLightFieldSurfaceGenerator::create(this);
	__makeGUI();
}

void App::__makeGUI()
{
	addWidget(m_pConfigWindow);
	m_pConfigWindow->pane()->pack();
	m_pConfigWindow->pack();
	m_pConfigWindow->setVisible(true);
	m_pConfigWindow->pane()->setVisible(true);
	
	showRenderingStats = false;
	debugWindow->setVisible(false);
	developerWindow->setVisible(false);
	developerWindow->cameraControlWindow->setVisible(false);
	developerWindow->sceneEditorWindow->moveTo(Vector2(0, 360.0f));
	developerWindow->sceneEditorWindow->setVisible(true);
}

void App::__refreshProbes(float vProbeRadius)
{
	SProbeStatus ProbeStatus = m_pConfigWindow->getProbeStatus();
	ProbeStatus.updateStatus();

	auto ProbeCounts = ProbeStatus.ProbeCounts;
	auto ProbeSteps = ProbeStatus.ProbeSteps;
	auto ProbeStartPos = ProbeStatus.ProbeStartPos;

	if (vProbeRadius <= 0) vProbeRadius = ProbeSteps.min() * 0.05f;

	for (int z = 0; z < ProbeCounts.z; ++z)
	{
		for (int y = 0; y < ProbeCounts.y; ++y)
		{
			for (int x = 0; x < ProbeCounts.x; ++x)
			{
				Color4 ProbeColor = Color4(x * 1.0f / ProbeCounts.x, y * 1.0f / ProbeCounts.y, z * 1.0f / ProbeCounts.z, 1.0f);
				auto ProbePos = ProbeStartPos + Vector3(x, y, z) * ProbeSteps;
				Draw::sphere(Sphere(ProbePos, vProbeRadius), renderDevice, ProbeColor, ProbeColor);
			}
		}
	}
}

void App::__enableEmissiveLight(bool vEnable)
{
	Array<shared_ptr<Entity>> EntityArray;
	scene()->getEntityArray(EntityArray);
	for (auto e : EntityArray)
	{
		if (e->name().find("emissiveLight")!=String::npos)
		{
			shared_ptr<VisibleEntity> EmissiveLight = dynamic_pointer_cast<VisibleEntity>(e);
			if (EmissiveLight)
				EmissiveLight->setVisible(vEnable);
		}
	}
}

void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>& allSurfaces)
{
	GApp::onGraphics3D(rd, allSurfaces);

	if (m_pGIRenderer->m_Settings.DisplayProbe) __refreshProbes();
}

shared_ptr<SLightFieldSurfaceMetaData> App::__initLightFieldSurfaceMetaData()
{
	shared_ptr<SLightFieldSurfaceMetaData> pMetaData = std::make_shared<SLightFieldSurfaceMetaData>();
	pMetaData->ProbeCubemapResolution = 1024;
	pMetaData->LightCubemapResolution = 1024;
	pMetaData->OctResolution  = 128;
	pMetaData->SphereSamplerSize = Vector2int32(64, 64);
	return pMetaData;
}

bool App::onEvent(const GEvent& event)
{
	if (event.type == GEventType::KEY_DOWN && event.key.keysym.sym == GKey::TAB)
	{
		m_pConfigWindow->setVisible(!m_pConfigWindow->visible());
		developerWindow->sceneEditorWindow->setVisible(m_pConfigWindow->visible());
	}
	
	return GApp::onEvent((event));
}

void App::onAfterLoadScene(const Any& any, const String& sceneName)
{
	Array<shared_ptr<Surface>> Surface;
	{
		Array<shared_ptr<Surface2D>> IgnoreSurface;
		onPose(Surface, IgnoreSurface);
	}

	SProbeStatus ProbeStatus = m_pConfigWindow->getProbeStatus();
	AABox BoundingBox;
	Surface::getBoxBounds(Surface, BoundingBox);

	ProbeStatus.BoundingBoxHigh = BoundingBox.high();
	ProbeStatus.BoundingBoxLow = BoundingBox.low();
	ProbeStatus.updateStatus();

	m_pConfigWindow->setProbeStatus(ProbeStatus);
}

void App::precompute()
{
	const SProbeStatus& ProbeStatus = m_pConfigWindow->getProbeStatus();

	m_pLightFieldSurfaceMetaData->ProbeCounts   = ProbeStatus.ProbeCounts;
	m_pLightFieldSurfaceMetaData->ProbeSteps    = ProbeStatus.ProbeSteps;
	m_pLightFieldSurfaceMetaData->ProbeStartPos = ProbeStatus.ProbeStartPos;

	m_pLightFieldSurface = m_pLightFieldSurfaceGenerator->generateLightFieldSurface(m_pLightFieldSurfaceMetaData);
	m_pGIRenderer->updateLightFieldSurface(m_pLightFieldSurface, m_pLightFieldSurfaceMetaData);

	m_renderer = m_pGIRenderer;
}

G3D_START_AT_MAIN();
int main(int argc, const char* argv[]) {
	initGLG3D();
	GApp::Settings settings(argc, argv);

	settings.window.caption = "Light Field Probe GI";
	settings.window.width = 1024;
	settings.window.height = 1024;
	settings.window.fullScreen = false;
	settings.window.resizable = false;
	settings.window.framed = !settings.window.fullScreen;
	settings.window.asynchronous = true;
	settings.window.defaultIconFilename = "icon.png";
	settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(0, 0);
	settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);

	settings.dataDir = FileSystem::currentDirectory();
	settings.screenCapture.outputDirectory = FileSystem::currentDirectory();
	settings.screenCapture.filenamePrefix = "_";
	settings.renderer.deferredShading = true;

	return App(settings).run();
}
