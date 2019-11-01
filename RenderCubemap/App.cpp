#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
	initGLG3D(G3DSpecification());

	GApp::Settings settings(argc, argv);

	settings.window.caption = argv[0];

	settings.window.defaultIconFilename = "icon.png";

	settings.window.width = 400;
	settings.window.height = 800;
	settings.window.fullScreen = false;
	settings.window.resizable = false;
	settings.window.framed = !settings.window.fullScreen;

	settings.window.asynchronous = false;

	settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(0, 0);
	settings.hdrFramebuffer.depthGuardBandThickness = settings.hdrFramebuffer.colorGuardBandThickness.max(Vector2int16(0, 0));

	settings.renderer.deferredShading = true;
	settings.renderer.orderIndependentTransparency = true;

	settings.dataDir = FileSystem::currentDirectory();

	settings.screenCapture.outputDirectory = "";
	settings.screenCapture.includeAppRevision = false;
	settings.screenCapture.includeG3DRevision = false;
	settings.screenCapture.filenamePrefix = "_";


	return App(settings).run();
}

App::App(const GApp::Settings& settings) : GApp(settings) {}

void App::onInit()
{
	GApp::onInit();

	setFrameDuration(1.0f / 240.0f);

	showRenderingStats = false;

	loadScene("G3D Sponza (Area Light)");

	m_pCubemapTexture = Texture::createEmpty("CubemapTexture", 1024, 1024, ImageFormat::RGB8(), Texture::DIM_CUBE_MAP, true);

	AABox BoundingBox;
	Surface::getBoxBounds(m_posed3D, BoundingBox);

	__renderLightFieldProbe(BoundingBox.center(), m_pCubemapTexture);

	scene()->clear();

	scene()->lightingEnvironment().environmentMapArray.append(m_pCubemapTexture);
	scene()->insert(Skybox::create("Skybox", &*scene(), scene()->lightingEnvironment().environmentMapArray, Array<SimTime>(0), 0.0f, SplineExtrapolationMode::CLAMP, false, false));
}

void App::__renderLightFieldProbe(const Point3& vPosition, shared_ptr<Texture> voCubemapTexture)
{
	Array<shared_ptr<Surface>> surface;
	{
		Array<shared_ptr<Surface2D> > ignore;
		onPose(surface, ignore);
	}

	const int oldFramebufferWidth = m_osWindowHDRFramebuffer->width();
	const int oldFramebufferHeight = m_osWindowHDRFramebuffer->height();
	const Vector2int16  oldColorGuard = m_settings.hdrFramebuffer.colorGuardBandThickness;
	const Vector2int16  oldDepthGuard = m_settings.hdrFramebuffer.depthGuardBandThickness;
	const shared_ptr<Camera>& oldCamera = activeCamera();

	_ASSERT(voCubemapTexture->width() == voCubemapTexture->height());
	auto Resolution = voCubemapTexture->width();

	m_settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(128,128);
	m_settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(256,256);
	const int fullWidth = Resolution + (2 * m_settings.hdrFramebuffer.depthGuardBandThickness.x);
	m_osWindowHDRFramebuffer->resize(fullWidth, fullWidth);

	shared_ptr<Camera> camera = Camera::create("Cubemap Camera");
	camera->copyParametersFrom(m_debugCamera);
	auto Position = vPosition;
	CFrame cf = CFrame::fromXYZYPRDegrees(Position.x, Position.y, Position.z);
	camera->setFrame(cf);

	camera->setTrack(nullptr);
	camera->depthOfFieldSettings().setEnabled(false);
	camera->motionBlurSettings().setEnabled(false);
	camera->setFieldOfView(2.0f * ::atan(1.0f + 2.0f*(float(m_settings.hdrFramebuffer.depthGuardBandThickness.x) / float(Resolution))), FOVDirection::HORIZONTAL);

	// Configure the base camera
	CFrame cframe = camera->frame();

	setActiveCamera(camera);
	for (int Face = 0; Face < 6; ++Face)
	{
		Texture::getCubeMapRotation(CubeFace(Face), cframe.rotation);
		camera->setFrame(cframe);

		renderDevice->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

		// Render every face twice to let the screen space reflection/refraction texture to stabilize
		GApp::onGraphics3D(renderDevice, surface);
		GApp::onGraphics3D(renderDevice, surface);

		Texture::copy(m_osWindowHDRFramebuffer->texture(0), voCubemapTexture, 0, 0, 1,
			Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - voCubemapTexture->vector2Bounds()) / 2.0f),
			CubeFace::POS_X, CubeFace(Face), nullptr, false);
	}

	setActiveCamera(oldCamera);
	m_osWindowHDRFramebuffer->resize(oldFramebufferWidth, oldFramebufferHeight);
	m_settings.hdrFramebuffer.colorGuardBandThickness = oldColorGuard;
	m_settings.hdrFramebuffer.depthGuardBandThickness = oldDepthGuard;
}
