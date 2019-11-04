#pragma once
#include "App.h"

void App::onInit()
{
	GApp::onInit();
	setFrameDuration(1.0f / 60.f);

	__specifyGBufferEncoding();

	loadScene("Simple Cornell Box");
	//loadScene("Simple Cornell Box (Mirror)");
	//loadScene("Sponza (Glossy Area Lights)");
	//loadScene("Sponza (Statue Glossy)");

	m_LightFieldSurface = __initLightFieldSurface();
	m_ProbePositionSet  = __placeProbe(m_LightFieldSurface);

	__enableEmissiveLight(false); {
		__precomputeLightFieldSurface(m_LightFieldSurface);
	} __enableEmissiveLight(true);

	m_pGIRenderer = dynamic_pointer_cast<CProbeGIRenderer>(CProbeGIRenderer::create(m_LightFieldSurface));
	m_pGIRenderer->setDeferredShading(true);
	m_renderer = m_pGIRenderer;
	
	__makeGUI();
	//m_debugController->setTurnRate(1);
	//m_debugController->setMoveRate(1);
}

void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& allSurfaces)
{
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

	GBuffer::Specification GbufferSpec = m_gbufferSpecification;
	extendGBufferSpecification(GbufferSpec);
	m_gbuffer->setSpecification(GbufferSpec);
	m_gbuffer->resize(m_framebuffer->width(), m_framebuffer->height());
	m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.hdrFramebuffer.depthGuardBandThickness, m_settings.hdrFramebuffer.colorGuardBandThickness);

	m_renderer->render(rd,
		activeCamera(),
		m_framebuffer,
		scene()->lightingEnvironment().ambientOcclusionSettings.enabled ? m_depthPeelFramebuffer : nullptr,
		scene()->lightingEnvironment(), m_gbuffer,
		allSurfaces);

	rd->pushState(m_framebuffer); {
		rd->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());
		drawDebugShapes();
		const shared_ptr<Entity>& selectedEntity = (notNull(developerWindow) && notNull(developerWindow->sceneEditorWindow)) ? developerWindow->sceneEditorWindow->selectedEntity() : nullptr;
		scene()->visualize(rd, selectedEntity, allSurfaces, sceneVisualizationSettings(), activeCamera());

		onPostProcessHDR3DEffects(rd);
	} rd->popState();

	if (submitToDisplayMode() == SubmitToDisplayMode::MAXIMIZE_THROUGHPUT) {
		swapBuffers();
	}

	rd->clear();

	m_film->exposeAndRender(rd, activeCamera()->filmSettings(), m_framebuffer->texture(0), settings().hdrFramebuffer.colorGuardBandThickness.x + settings().hdrFramebuffer.depthGuardBandThickness.x, settings().hdrFramebuffer.depthGuardBandThickness.x,
		Texture::opaqueBlackIfNull(notNull(m_gbuffer) ? m_gbuffer->texture(GBuffer::Field::SS_POSITION_CHANGE) : nullptr),
		activeCamera()->jitterMotion());
}


shared_ptr<Texture> App::__createSphereSampler(int vDegreeSize /*= 64*/)
{
	std::vector<Vector4> UniformSampleDirectionsOnSphere;
	UniformSampleDirectionsOnSphere.resize(square(vDegreeSize));

	float PerDegree = 360.0f / vDegreeSize;
	float RadianSeta = 0.0f, RadianFai = 0.0f;

	for (int i = 0; i < vDegreeSize; i++)
	{
		for (int k = 0; k < vDegreeSize; k++)
		{
			RadianSeta = toRadians(k*PerDegree*0.5f);
			RadianFai = toRadians(i*PerDegree);
			UniformSampleDirectionsOnSphere[i*vDegreeSize + k] = Vector4(
				sin(RadianSeta)*cos(RadianFai),
				sin(RadianSeta)*sin(RadianFai),
				cos(RadianSeta),
				0.0);
		}
	}

	return Texture::fromMemory("SphereSampler", UniformSampleDirectionsOnSphere.data(), ImageFormat::RGBA32F(), vDegreeSize, vDegreeSize, 1, 1, ImageFormat::RGBA16F(), Texture::DIM_2D, false);
}

void App::__makeGUI()
{
	debugWindow->setVisible(true);
	showRenderingStats = false;

	debugPane->beginRow(); {
		debugPane->addCheckBox("Direct", &m_pGIRenderer->m_Settings.Direct)->moveBy(10, 0);
		debugPane->addCheckBox("Indirect Diffuse", &m_pGIRenderer->m_Settings.IndirectDiffuse)->moveBy(20, 0);
		debugPane->addCheckBox("Indirect Glossy", &m_pGIRenderer->m_Settings.IndirectGlossy)->moveBy(30, 0);
		debugPane->addCheckBox("Display Probe", &m_pGIRenderer->m_Settings.DisplayProbe)->moveBy(40, 0);
	} debugPane->endRow();

	debugWindow->pack();
	debugWindow->setRect(Rect2D::xywh(0, 0, (float)window()->width(), debugWindow->rect().height()));
	developerWindow->cameraControlWindow->moveTo(Vector2(0, 450));
	developerWindow->cameraControlWindow->setVisible(false);
	developerWindow->setVisible(false);
	developerWindow->sceneEditorWindow->moveTo(Point2int16(0, 50));
}

void App::__specifyGBufferEncoding()
{
	m_gbufferSpecification.encoding[GBuffer::Field::CS_POSITION] = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::WS_POSITION] = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::WS_NORMAL]   = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::GLOSSY]      = ImageFormat::RGBA32F();
	m_gbufferSpecification.encoding[GBuffer::Field::CS_NORMAL]   = Texture::Encoding(ImageFormat::RGB10A2(), FrameName::CAMERA, 2.0f, -1.0f);
	m_gbufferSpecification.encoding[GBuffer::Field::DEPTH_AND_STENCIL]  = ImageFormat::DEPTH32F();
	m_gbufferSpecification.encoding[GBuffer::Field::SS_POSITION_CHANGE] = ImageFormat::RGB32F();
}

void App::__precomputeLightFieldSurface(SLightFieldSurface& vioLightFieldSurface)
{
	shared_ptr<Texture> SphereSamplerTexture = __createSphereSampler();
	SLightFieldCubemap LightFieldCubemap(1024);
	shared_ptr<Framebuffer> LightFieldFramebuffer = Framebuffer::create("LightFieldFB");

	for (int i = 0; i < m_ProbePositionSet.size(); ++i)
	{
		__renderLightFieldProbe2Cubemap(i, 1024, LightFieldCubemap);

		LightFieldFramebuffer->set(Framebuffer::COLOR0, vioLightFieldSurface.RadianceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR1, vioLightFieldSurface.DistanceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR2, vioLightFieldSurface.NormalProbeGrid, CubeFace::POS_X, 0, i);
		__generateHighResOctmap(LightFieldFramebuffer, LightFieldCubemap);

		LightFieldFramebuffer->set(Framebuffer::COLOR0, vioLightFieldSurface.IrradianceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR1, vioLightFieldSurface.MeanDistProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR2, vioLightFieldSurface.LowResolutionDistanceProbeGrid, CubeFace::POS_X, 0, i);
		__generateLowResOctmap(LightFieldFramebuffer, LightFieldCubemap, SphereSamplerTexture);
	}
}

void App::__generateLowResOctmap(std::shared_ptr<G3D::Framebuffer>& vLightFieldFramebuffer, SLightFieldCubemap& vLightFieldCubemap, shared_ptr<G3D::Texture>& vSphereSamplerTexture)
{
	auto CubemapSampler = Sampler::cubeMap();
	CubemapSampler.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;

	renderDevice->push2D(vLightFieldFramebuffer); {
		renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		Args args;
		args.setRect(Rect2D(Point2(0, 0), Point2(128, 128)));
		args.setUniform("RadianceCubemap", vLightFieldCubemap.RadianceCubemap, CubemapSampler);
		args.setUniform("DistanceCubemap", vLightFieldCubemap.DistanceCubemap, CubemapSampler);
		args.setUniform("NumSamples", 2048);
		args.setUniform("OctmapResolution", 128);
		args.setUniform("SphereSampler", vSphereSamplerTexture, Sampler::buffer());

		LAUNCH_SHADER("GenerateLowResOctmap.pix", args);
	} renderDevice->pop2D();
}

void App::__generateHighResOctmap(std::shared_ptr<G3D::Framebuffer>& vLightFieldFramebuffer, SLightFieldCubemap& vLightFieldCubemap)
{
	auto CubemapSampler = Sampler::cubeMap();
	CubemapSampler.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;

	renderDevice->push2D(vLightFieldFramebuffer); {
		renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);

		Args args;
		args.setRect(Rect2D(Point2(0, 0), Point2(1024, 1024)));
		args.setUniform("RadianceCubemap", vLightFieldCubemap.RadianceCubemap, CubemapSampler);
		args.setUniform("DistanceCubemap", vLightFieldCubemap.DistanceCubemap, CubemapSampler);
		args.setUniform("NormalCubemap", vLightFieldCubemap.NormalCubemap, CubemapSampler);
		args.setUniform("OctmapResolution", 1024);

		LAUNCH_SHADER("GenerateHighResOctmap.pix", args);
	} renderDevice->pop2D();
}

SLightFieldSurface App::__initLightFieldSurface()
{
	SLightFieldSurface LightFieldSurface;

	AABox BoundingBox;
	Surface::getBoxBounds(m_posed3D, BoundingBox);

	auto BoundingBoxRange = BoundingBox.high() - BoundingBox.low();

	auto Bounds = BoundingBoxRange * 0.2f;//NOTE: range from 0.1 to 0.5

	LightFieldSurface.ProbeCounts = Vector3int32(2,2,2);
	LightFieldSurface.ProbeSteps = (BoundingBoxRange - Bounds * 2) / (LightFieldSurface.ProbeCounts - Vector3int32(1, 1, 1));
	LightFieldSurface.ProbeStartPosition = BoundingBox.low() + Bounds;
	if (LightFieldSurface.ProbeCounts.x == 1) { LightFieldSurface.ProbeSteps.x = BoundingBoxRange.x * 0.5; LightFieldSurface.ProbeStartPosition.x = BoundingBox.low().x + BoundingBoxRange.x * 0.5; } 
	if (LightFieldSurface.ProbeCounts.y == 1) { LightFieldSurface.ProbeSteps.y = BoundingBoxRange.y * 0.5; LightFieldSurface.ProbeStartPosition.y = BoundingBox.low().y + BoundingBoxRange.y * 0.5; } 
	if (LightFieldSurface.ProbeCounts.z == 1) { LightFieldSurface.ProbeSteps.z = BoundingBoxRange.z * 0.5; LightFieldSurface.ProbeStartPosition.z = BoundingBox.low().z + BoundingBoxRange.z * 0.5; } 

	auto ProbeNum = LightFieldSurface.ProbeCounts.x * LightFieldSurface.ProbeCounts.y * LightFieldSurface.ProbeCounts.z;

	LightFieldSurface.RadianceProbeGrid   = Texture::createEmpty("RaidanceProbeGrid", 1024, 1024, ImageFormat::R11G11B10F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	LightFieldSurface.DistanceProbeGrid   = Texture::createEmpty("DistanceProbeGrid", 1024, 1024, ImageFormat::R16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	LightFieldSurface.NormalProbeGrid     = Texture::createEmpty("NormalProbeGrid", 1024, 1024, ImageFormat::RGB16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	LightFieldSurface.IrradianceProbeGrid = Texture::createEmpty("IrraidanceProbeGrid", 128, 128, ImageFormat::R11G11B10F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	LightFieldSurface.MeanDistProbeGrid   = Texture::createEmpty("MeanDistProbeGrid", 128, 128, ImageFormat::RG16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	LightFieldSurface.LowResolutionDistanceProbeGrid = Texture::createEmpty("LowResolutionDistanceProbeGrid", 128, 128, ImageFormat::R16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);

	return LightFieldSurface;
}

std::vector<Vector3> App::__placeProbe(const SLightFieldSurface& vLightFieldSurface)
{
	std::vector<Vector3> ProbePositionSet;

	auto ProbeCounts   = vLightFieldSurface.ProbeCounts;
	auto ProbeSteps    = vLightFieldSurface.ProbeSteps;
	auto ProbeStartPos = vLightFieldSurface.ProbeStartPosition;

	for (int z = 0; z < ProbeCounts.z; ++z)
	{
		for (int y = 0; y < ProbeCounts.y; ++y)
		{
			for (int x = 0; x < ProbeCounts.x; ++x)
			{
				ProbePositionSet.emplace_back(Vector3(
					ProbeStartPos.x + x * ProbeSteps.x,
					ProbeStartPos.y + y * ProbeSteps.y,
					ProbeStartPos.z + z * ProbeSteps.z
				));
			}
		}
	}

	return ProbePositionSet;
}

void App::__renderLightFieldProbe2Cubemap(uint32 vProbeIndex, int vResolution, SLightFieldCubemap& voLightFieldCubemap)
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
	const int fullWidth = vResolution + (2 * m_settings.hdrFramebuffer.depthGuardBandThickness.x);
	m_osWindowHDRFramebuffer->resize(fullWidth, fullWidth);

	shared_ptr<Camera> CubemapCamera = Camera::create("Cubemap Camera");
	CubemapCamera->copyParametersFrom(activeCamera());
	CubemapCamera->setTrack(nullptr);
	CubemapCamera->depthOfFieldSettings().setEnabled(false);
	CubemapCamera->motionBlurSettings().setEnabled(false);
	CubemapCamera->setFieldOfView(2.0f * ::atan(1.0f + 2.0f*(float(m_settings.hdrFramebuffer.depthGuardBandThickness.x) / float(vResolution))), FOVDirection::HORIZONTAL);
	CubemapCamera->setNearPlaneZ(-0.001f);
	CubemapCamera->setFarPlaneZ(-1000.0f);
	setActiveCamera(CubemapCamera);

	auto Position = m_ProbePositionSet[vProbeIndex];
	CFrame Frame = CFrame::fromXYZYPRDegrees(Position.x, Position.y, Position.z);

	for (int Face = 0; Face < 6; ++Face) {
		Texture::getCubeMapRotation(CubeFace(Face), Frame.rotation);
		CubemapCamera->setFrame(Frame);

		renderDevice->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

		onGraphics3D(renderDevice, Surface);

		Texture::copy(m_osWindowHDRFramebuffer->texture(0), voLightFieldCubemap.RadianceCubemap, 0, 0, 1,
					  Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - voLightFieldCubemap.RadianceCubemap->vector2Bounds()) / 2.0f),
					  CubeFace::POS_X, CubeFace(Face) , nullptr, false);

		Texture::copy(m_gbuffer->texture(GBuffer::Field::CS_POSITION), voLightFieldCubemap.DistanceCubemap, 0, 0, 1,
					  Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - voLightFieldCubemap.DistanceCubemap->vector2Bounds()) / 2.0f),
					  CubeFace::POS_X, CubeFace(Face), nullptr, false);

		Texture::copy(m_gbuffer->texture(GBuffer::Field::WS_NORMAL), voLightFieldCubemap.NormalCubemap,0, 0, 1,
					  Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - voLightFieldCubemap.NormalCubemap->vector2Bounds()) / 2.0f),
					  CubeFace::POS_X, CubeFace(Face), nullptr, false);
	}

	setActiveCamera(OldCamera);
	m_osWindowHDRFramebuffer->resize(OldFramebufferWidth, OldFramebufferHeight);
	m_settings.hdrFramebuffer.colorGuardBandThickness = OldColorGuard;
	m_settings.hdrFramebuffer.depthGuardBandThickness = OldDepthGuard;
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

G3D_START_AT_MAIN();
int main(int argc, const char* argv[]) {
	initGLG3D();
	GApp::Settings settings(argc, argv);

	settings.window.caption = "Light Field Probe GI";
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
