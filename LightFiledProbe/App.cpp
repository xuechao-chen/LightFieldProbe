#pragma once
#include <G3D/G3D.h>
#include "ProbeGIRenderer.h"

class App : public GApp
{
public:
	App(const GApp::Settings& vSettings) : GApp(vSettings) 
	{
	}

	virtual void onInit() override
	{
		GApp::onInit();
		setFrameDuration(1.0f / 60.f);

		__makeGUI();

		logPrintf("Program initialized\n");
		loadScene("Animated Hardening");
		logPrintf("Loaded Scene\n");

		AABox BoundingBox;
		Surface::getBoxBounds(m_posed3D, BoundingBox);

		m_pGIRenderer = dynamic_pointer_cast<CProbeGIRenderer>(CProbeGIRenderer::create(BoundingBox));
		m_pGIRenderer->setDeferredShading(true);
		m_renderer = m_pGIRenderer;
	}

private:
	void __makeGUI()
	{
		debugWindow->setVisible(false);
		showRenderingStats = false;
	}

	shared_ptr<CProbeGIRenderer> m_pGIRenderer;
};

G3D_START_AT_MAIN();
int main(int argc, const char* argv[]) {
	initGLG3D();
	GApp::Settings settings(argc, argv);

	settings.window.caption = "Light Filed Probe GI";
	settings.window.width = 1024; settings.window.height = 1024;
	settings.window.fullScreen = false;
	settings.window.resizable = false;
	settings.window.framed = !settings.window.fullScreen;
	settings.window.asynchronous = false;
	settings.window.defaultIconFilename = "icon.png";
	settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(0, 0);
	settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);

	settings.dataDir = FileSystem::currentDirectory();
	settings.screenCapture.outputDirectory = FileSystem::currentDirectory();
	settings.screenCapture.filenamePrefix = "_";
	settings.renderer.deferredShading = true;

	return App(settings).run();
}
