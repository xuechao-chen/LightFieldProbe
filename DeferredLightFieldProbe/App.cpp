#pragma once
#include "App.h"
#include "ConfigWindow.h"
#include "HammersleySampler.h"
#include "LightFieldSurfaceGenerator.h"

void App::onInit()
{
	std::vector<String> AllScenes = {
		/* 0 */ "Simple Cornell Box",
		/* 1 */ "Simple Cornell Box (No Little Boxes)",
		/* 2 */ "Sponza (Glossy Area Lights)",
		/* 3 */ "Sponza (Statue Glossy)",
		/* 4 */ "Dragon",
		/* 5 */ "Dragon Low"
	};

	GApp::onInit();
	setFrameDuration(1.0f / 60.f);
	
	m_pDefaultRenderer = m_renderer;
	m_pGIRenderer = dynamic_pointer_cast<CProbeGIRenderer>(CProbeGIRenderer::create());
	m_pGIRenderer->setDeferredShading(true);

	m_pConfigWindow = CConfigWindow::create(this);

	m_pHighResScene = Scene::create(m_ambientOcclusion);
	m_pLowResScene = Scene::create(m_ambientOcclusion);
	loadScene(AllScenes[4], AllScenes[5]);
	useHighResScene();
	//loadScene(AllScenes[5].c_str());
	
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
	SProbeStatus& ProbeStatus = m_pConfigWindow->fetchProbeStatus();
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

void App::__updateScene(const shared_ptr<Scene>& vScene)
{
	setScene(vScene);

	// Trigger one frame of rendering, to force shaders to load and compile
	m_posed3D.fastClear();
	m_posed2D.fastClear();
	if (scene()) {
		onPose(m_posed3D, m_posed2D);
	}

	onGraphics(renderDevice, m_posed3D, m_posed2D);

	// Reset our idea of "now" so that simulation doesn't see a huge lag
	// due to the scene load time.
	m_lastTime = m_now = System::time() - 0.0001f;
}

void App::loadScene(const String& vHighResSceneName, const String& vLowResSceneName)
{
	drawMessage("Loading Scene " + vHighResSceneName + " and " + vLowResSceneName);

	m_pHighResScene->clear();
	m_pLowResScene->clear();
	
	const String oldSceneName = m_pHighResScene->name();

	// Load the scene
	try {
		m_activeCameraMarker->setTrack(nullptr);
		const Any& HighResAny = m_pHighResScene->load(vHighResSceneName);
		const Any& LowResAny = m_pLowResScene->load(vLowResSceneName);

		// If the debug camera was active and the scene is the same as before, retain the old camera.  Otherwise,
		// switch to the default camera specified by the scene.

		if ((oldSceneName != vHighResSceneName) || (activeCamera()->name() != "(Debug Camera)")) {

			// Because the CameraControlWindow is hard-coded to the
			// m_debugCamera, we have to copy the camera's values here
			// instead of assigning a pointer to it.
			m_debugCamera->copyParametersFrom(m_pHighResScene->defaultCamera());
			m_debugCamera->setTrack(nullptr);
			m_debugController->setFrame(m_debugCamera->frame());

			setActiveCamera(m_pHighResScene->defaultCamera());
		}
		// Re-insert the active camera marker
		m_pHighResScene->insert(m_activeCameraMarker);
		__updateScene(m_pHighResScene);
		onAfterLoadScene(HighResAny, vHighResSceneName);
	}
	catch (const ParseError& e) {
		const String& msg = e.filename + format(":%d(%d): ", e.line, e.character) + e.message;
		debugPrintf("%s", msg.c_str());
		logPrintf("%s", msg.c_str());
		drawMessage(msg);
		System::sleep(5);
		m_pHighResScene->clear();
		m_pLowResScene->clear();
		m_pHighResScene->lightingEnvironment().ambientOcclusion = m_ambientOcclusion;
		m_pLowResScene->lightingEnvironment().ambientOcclusion = m_ambientOcclusion;
	}
}

void App::onAfterLoadScene(const Any& any, const String& sceneName)
{
	Array<shared_ptr<Surface>> Surface;
	{
		Array<shared_ptr<Surface2D>> IgnoreSurface;
		onPose(Surface, IgnoreSurface);
	}

	SProbeStatus& ProbeStatus = m_pConfigWindow->fetchProbeStatus();
	AABox BoundingBox;
	Surface::getBoxBounds(Surface, BoundingBox);

	ProbeStatus.BoundingBoxHigh = BoundingBox.high();
	ProbeStatus.BoundingBoxLow = BoundingBox.low();
	ProbeStatus.updateStatus();
}

void App::precompute()
{
	const SProbeStatus& ProbeStatus = m_pConfigWindow->fetchProbeStatus();

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
