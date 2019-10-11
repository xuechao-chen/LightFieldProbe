#pragma once
#include <G3D/G3D.h>

class App : public GApp
{
public:
	App(const GApp::Settings& vSettings) : GApp(vSettings) {}

	virtual void onInit() override
	{
		GApp::onInit();
		setFrameDuration(1.0f / 60.f);

		loadScene("Teapot on Metal");
	}
};

G3D_START_AT_MAIN();
int main(int argc, const char* argv[]) {
	initGLG3D();
	GApp::Settings settings(argc, argv);

	settings.window.caption = "Light Filed Probe GI";
	settings.window.width = 1024; settings.window.height = 800;
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
	settings.renderer.orderIndependentTransparency = true;

	return App(settings).run();
}
