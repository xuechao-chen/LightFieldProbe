#include "App.h"
#include "../LightFiledProbe/HammersleySampler.h"

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
	m_gbufferSpecification.encoding[GBuffer::Field::WS_POSITION] = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::WS_NORMAL] = ImageFormat::RGB32F();

	loadScene("Simple Cornell Box (No Little Boxes)");
	//loadScene("Sponza (Glossy Area Lights)");

	m_pRadianceCubemap = Texture::createEmpty("RadianceCubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);
	m_pDistanceCubemap = Texture::createEmpty("DistanceCubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);
	m_pNormalCubemap   = Texture::createEmpty("NormalCubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);

	AABox BoundingBox;
	Surface::getBoxBounds(m_posed3D, BoundingBox);

	const int LowResolution = 2048;
	shared_ptr<Texture> LowResRadianceCubemap = Texture::createEmpty("RadianceCubemap", LowResolution, LowResolution, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);
	shared_ptr<Texture> LowResDistanceCubemap = Texture::createEmpty("DistanceCubemap", LowResolution, LowResolution, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);
	shared_ptr<Texture> LowResNormalCubemap = Texture::createEmpty("NormalCubemap", LowResolution, LowResolution, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);

	__renderLightFieldProbe(1024,Point3(0,1,0), m_pRadianceCubemap, m_pDistanceCubemap, m_pNormalCubemap);
	shared_ptr<Texture> RadianceOctFromLight = generateOctmap(m_pRadianceCubemap, 1024);

	__renderLightFieldProbe(1024,Point3(0, 1, 0), m_pRadianceCubemap, m_pDistanceCubemap, m_pNormalCubemap);
	m_pRadianceMapFromProbe = generateOctmap(m_pRadianceCubemap, 1024);
	
	__renderLightFieldProbe(LowResolution, Point3(0, 1, 0), LowResRadianceCubemap, LowResDistanceCubemap, LowResNormalCubemap);
	m_pReconstructRadianceMapFromProbe = reconstructRadianceMap(LowResDistanceCubemap, RadianceOctFromLight);

	m_IsInit = true;
}

shared_ptr<Texture> App::generateOctmap(shared_ptr<Texture> vCubemapTexture, int vResolution)
{
	auto CubemapSampler = Sampler::cubeMap();
	CubemapSampler.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;

	shared_ptr<Framebuffer> TempFramebuffer = Framebuffer::create("Framebuffer");
	TempFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("Octmap", vResolution, vResolution, ImageFormat::RGB32F()));

	renderDevice->push2D(TempFramebuffer); {
		renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);

		Args args;
		args.setRect(Rect2D(Point2(0, 0), Point2(vResolution, vResolution)));
		args.setUniform("Cubemap", vCubemapTexture, CubemapSampler);

		LAUNCH_SHADER("GenerateOctmap.pix", args);
	} renderDevice->pop2D();

	return TempFramebuffer->texture(0);
}

shared_ptr<Texture> App::reconstructRadianceMap(shared_ptr<Texture> vPositionCubemap, shared_ptr<Texture> vRadianceMapFromLight)
{
	shared_ptr<Framebuffer> TempFramebuffer = Framebuffer::create("Framebuffer");
	TempFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("Octmap", 1024, 1024, ImageFormat::RGB32F()));

	auto CubemapSampler = Sampler::cubeMap();
	CubemapSampler.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;

	renderDevice->push2D(TempFramebuffer); {
		renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);

		Args args;
		args.setRect(Rect2D(Point2(0, 0), Point2(1024, 1024)));
		args.setUniform("PositionCubemap", vPositionCubemap, CubemapSampler);
		args.setUniform("RadianceMapFromLight", vRadianceMapFromLight, Sampler::buffer());

		LAUNCH_SHADER("ReconstructRadianceMap.pix", args);
	} renderDevice->pop2D();

	return TempFramebuffer->texture(0);
}

void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>& allSurfaces)
{
	if (!m_IsInit) {
		if (!scene()) {
			if ((submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) && (!rd->swapBuffersAutomatically())) {
				swapBuffers();
			}
			rd->clear();
			rd->pushState(); {
				rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
				drawDebugShapes();
			} rd->popState();
			return;
		}

		BEGIN_PROFILER_EVENT("GApp::onGraphics3D");
		GBuffer::Specification gbufferSpec = m_gbufferSpecification;
		extendGBufferSpecification(gbufferSpec);
		m_gbuffer->setSpecification(gbufferSpec);

		const Vector2int32 framebufferSize = m_settings.hdrFramebuffer.hdrFramebufferSizeFromDeviceSize(Vector2int32(m_deviceFramebuffer->vector2Bounds()));
		m_framebuffer->resize(framebufferSize);
		m_gbuffer->resize(framebufferSize);
		m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.hdrFramebuffer.depthGuardBandThickness, m_settings.hdrFramebuffer.colorGuardBandThickness);

		m_renderer->render(rd, activeCamera(), m_framebuffer, scene()->lightingEnvironment().ambientOcclusionSettings.enabled ? m_depthPeelFramebuffer : nullptr,
			scene()->lightingEnvironment(), m_gbuffer, allSurfaces);

		// Debug visualizations and post-process effects
		rd->pushState(m_framebuffer); {
			// Call to make the App show the output of debugDraw(...)
			rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
			drawDebugShapes();
			const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : nullptr;
			scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings(), activeCamera());

			onPostProcessHDR3DEffects(rd);
		} rd->popState();

		// We're about to render to the actual back buffer, so swap the buffers now.
		// This call also allows the screenshot and video recording to capture the
		// previous frame just before it is displayed.
		if (submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) {
			swapBuffers();
		}

		// Clear the entire screen (needed even though we'll render over it, since
		// AFR uses clear() to detect that the buffer is not re-used.)
		BEGIN_PROFILER_EVENT("RenderDevice::clear");
		rd->clear();
		END_PROFILER_EVENT();

		// Perform gamma correction, bloom, and SSAA, and write to the native window frame buffer
		m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0), settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.depthGuardBandThickness.x,
			Texture::opaqueBlackIfNull(notNull(m_gbuffer) ? m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE) : nullptr),
			activeCamera()->jitterMotion());
		END_PROFILER_EVENT();
		return;
	}

	rd->swapBuffers();
	rd->clear();

	rd->push2D(); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);

		Args args;
		args.setRect(Rect2D(Point2(0, 0), Point2(1024, 1024)));
		args.setUniform("IndirectDiffuseTexture", m_pRadianceMapFromProbe, Sampler::buffer());
		args.setUniform("IndirectGlossyTexture", m_pReconstructRadianceMapFromProbe, Sampler::buffer());

		LAUNCH_SHADER("Merge.pix", args);
	} rd->pop2D();

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

void App::__renderLightFieldProbe(int vResolution, const Point3& vPosition, shared_ptr<Texture> voRadianceCubemap, shared_ptr<Texture> voDistanceCubemap, shared_ptr<Texture> voNormalCubemap)
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
	CubemapCamera->setNearPlaneZ(-0.001);
	CubemapCamera->setFarPlaneZ(-1000);


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

		Texture::copy(m_gbuffer->texture(GBuffer::Field::WS_POSITION), voDistanceCubemap, 0, 0, 1,
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
