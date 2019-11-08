#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[]) {
	initGLG3D(G3DSpecification());

	GApp::Settings settings(argc, argv);

	settings.window.caption = argv[0];

	settings.window.defaultIconFilename = "icon.png";

	settings.window.width = 1024;
	settings.window.height = 1024;
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

	m_gbufferSpecification.encoding[GBuffer::Field::CS_POSITION] = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::WS_NORMAL] = ImageFormat::RGB32F();

	//loadScene("Simple Cornell Box");
	loadScene("Sponza (Glossy Area Lights)");

	m_pRadianceCubemap = Texture::createEmpty("RadianceCubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);
	m_pDistanceCubemap = Texture::createEmpty("DistanceCubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);
	m_pNormalCubemap = Texture::createEmpty("NormalCubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);

	AABox BoundingBox;
	Surface::getBoxBounds(m_posed3D, BoundingBox);

	__renderLightFieldProbe(BoundingBox.center(), m_pRadianceCubemap, m_pDistanceCubemap, m_pNormalCubemap);

	__makeGUI();

	scene()->clear();
	scene()->lightingEnvironment().environmentMapArray.append(m_pRadianceCubemap);
	scene()->insert(Skybox::create("Skybox", &*scene(), scene()->lightingEnvironment().environmentMapArray, Array<SimTime>(0), 0.0f, SplineExtrapolationMode::CLAMP, false, false));
}

void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>& allSurfaces)
{
	if (!scene())
	{
		if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!rd->swapBuffersAutomatically()))
		{
			swapBuffers();
		}
		rd->clear();
		rd->pushState(); {
			rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
			drawDebugShapes();
		} rd->popState();
		return;
	}

	GBuffer::Specification GbufferSpec = m_gbufferSpecification;
	extendGBufferSpecification(GbufferSpec);
	m_gbuffer->setSpecification(GbufferSpec);
	m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());
	m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.hdrFramebuffer.depthGuardBandThickness, m_settings.hdrFramebuffer.colorGuardBandThickness);

	m_renderer->render(rd, activeCamera(), m_framebuffer, scene()->lightingEnvironment().ambientOcclusionSettings.enabled ? m_depthPeelFramebuffer : nullptr, scene()->lightingEnvironment(), m_gbuffer, allSurfaces);

	rd->pushState(m_framebuffer); {
		rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
		drawDebugShapes();
		const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : nullptr;
		scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings(), activeCamera());

		onPostProcessHDR3DEffects(rd);
	} rd->popState();

	if (submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT)
	{
		swapBuffers();
	}

	rd->clear();

	m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0), settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.depthGuardBandThickness.x,
		Texture::opaqueBlackIfNull(notNull(m_gbuffer) ? m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE) : nullptr), activeCamera()->jitterMotion());
}

void App::__makeGUI()
{
	debugWindow->setVisible(true);

	static int Index = 0;
	static Array<String> CubemapList("Radiance", "Distance", "Normal");

	debugPane->beginRow(); {
		debugPane->addDropDownList("Cubemap", CubemapList, &Index, [&]() {
			const String& Name = CubemapList[Index];
			shared_ptr<Texture> CurrentCubemap = m_pRadianceCubemap;

			if (Name == "Radiance") {
				CurrentCubemap = m_pRadianceCubemap;
			}
			else if (Name == "Distance") {
				CurrentCubemap = m_pDistanceCubemap;
			}
			else {
				CurrentCubemap = m_pNormalCubemap;
			}

			scene()->clear();
			scene()->lightingEnvironment().environmentMapArray.append(CurrentCubemap);
			scene()->insert(Skybox::create("Skybox", &*scene(), scene()->lightingEnvironment().environmentMapArray, Array<SimTime>(0), 0.0f, SplineExtrapolationMode::CLAMP, false, false));
		});
	} debugPane->endRow();

	debugWindow->pack();
}

void App::__renderLightFieldProbe(const Point3& vPosition, shared_ptr<Texture> voRadianceCubemap, shared_ptr<Texture> voDistanceCubemap, shared_ptr<Texture> voNormalCubemap)
{
	Array<shared_ptr<Surface>> Surface;
	{
		Array<shared_ptr<Surface2D>> IgnoreSurface;
		onPose(Surface, IgnoreSurface);
	}

	const int OldFramebufferWidth = m_osWindowHDRFramebuffer->width();
	const int OldFramebufferHeight = m_osWindowHDRFramebuffer->height();
	const Vector2int16  OldColorGuard = m_settings.hdrFramebuffer.colorGuardBandThickness;
	const Vector2int16  OldDepthGuard = m_settings.hdrFramebuffer.depthGuardBandThickness;
	const shared_ptr<Camera>& OldCamera = activeCamera();

	m_settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(128, 128);
	m_settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(256, 256);
	const int fullWidth = voRadianceCubemap->width() + (2 * m_settings.hdrFramebuffer.depthGuardBandThickness.x);
	m_osWindowHDRFramebuffer->resize(fullWidth, fullWidth);

	shared_ptr<Camera> CubemapCamera = Camera::create("Cubemap Camera");
	CubemapCamera->copyParametersFrom(activeCamera());
	CubemapCamera->setTrack(nullptr);
	CubemapCamera->depthOfFieldSettings().setEnabled(false);
	CubemapCamera->motionBlurSettings().setEnabled(false);
	CubemapCamera->setFieldOfView(2.0f * ::atan(1.0f + 2.0f*(float(m_settings.hdrFramebuffer.depthGuardBandThickness.x) / float(voRadianceCubemap->width()))), FOVDirection::HORIZONTAL);

	setActiveCamera(CubemapCamera);

	auto Position = vPosition;
	CFrame Frame = CFrame::fromXYZYPRDegrees(Position.x, Position.y, Position.z);

	for (int Face = 0; Face < 6; ++Face) {
		Texture::getCubeMapRotation(CubeFace(Face), Frame.rotation);
		CubemapCamera->setFrame(Frame);

		renderDevice->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

		onGraphics3D(renderDevice, Surface);

		Texture::copy(m_osWindowHDRFramebuffer->texture(0), voRadianceCubemap, 0, 0, 1,
			Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - voRadianceCubemap->vector2Bounds()) / 2.0f),
			CubeFace::POS_X, CubeFace(Face), nullptr, false);

		Texture::copy(m_gbuffer->texture(GBuffer::Field::CS_POSITION), voDistanceCubemap, 0, 0, 1,
			Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - voDistanceCubemap->vector2Bounds()) / 2.0f),
			CubeFace::POS_X, CubeFace(Face), nullptr, false);

		Texture::copy(m_gbuffer->texture(GBuffer::Field::WS_NORMAL), voNormalCubemap, 0, 0, 1,
			Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - voNormalCubemap->vector2Bounds()) / 2.0f),
			CubeFace::POS_X, CubeFace(Face), nullptr, false);
	}

	setActiveCamera(OldCamera);
	m_osWindowHDRFramebuffer->resize(OldFramebufferWidth, OldFramebufferHeight);
	m_settings.hdrFramebuffer.colorGuardBandThickness = OldColorGuard;
	m_settings.hdrFramebuffer.depthGuardBandThickness = OldDepthGuard;
}
