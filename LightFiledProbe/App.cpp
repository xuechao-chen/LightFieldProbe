#pragma once
#include "App.h"
#include "ConfigWindow.h"

void App::onInit()
{
	GApp::onInit();
	setFrameDuration(1.0f / 60.f);

	__specifyGBufferEncoding();

	//loadScene("Simple Cornell Box");
	//loadScene("Simple Cornell Box (Mirror)");
	loadScene("Simple Cornell Box (No Little Boxes)");

	//loadScene("Sponza (Glossy Area Lights)");
	//loadScene("Sponza (Statue Glossy)");

	m_pProbeStatus = __initProbeStatus();
	m_DefaultRenderer = m_renderer;
	precompute();

	m_pGIRenderer = dynamic_pointer_cast<CProbeGIRenderer>(CProbeGIRenderer::create(m_pLightFieldSurface, m_pProbeStatus));
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
	m_pConfigWindow = CConfigWindow::create(this, m_pGIRenderer, m_pProbeStatus);
	addWidget(m_pConfigWindow);
	m_pConfigWindow->pane()->pack();
	m_pConfigWindow->pack();
	m_pConfigWindow->setVisible(true);
	m_pConfigWindow->pane()->setVisible(true);
	
	showRenderingStats = false;
	debugWindow->setVisible(false);
	developerWindow->setVisible(false);
	developerWindow->cameraControlWindow->setVisible(false);
	developerWindow->sceneEditorWindow->setVisible(false);
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

void App::__precomputeLightFieldSurface(const shared_ptr<SLightFieldSurface>& vioLightFieldSurface)
{

	
	shared_ptr<Texture> SphereSamplerTexture = __createSphereSampler();
	SLightFieldCubemap LightFieldCubemap(1024);
	shared_ptr<Framebuffer> LightFieldFramebuffer = Framebuffer::create("LightFieldFB");

	for (int i = 0; i < m_ProbePositionSet.size(); ++i)
	{
		__renderLightFieldProbe2Cubemap(i, 1024, LightFieldCubemap);

		LightFieldFramebuffer->set(Framebuffer::COLOR0, vioLightFieldSurface->RadianceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR1, vioLightFieldSurface->DistanceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR2, vioLightFieldSurface->NormalProbeGrid, CubeFace::POS_X, 0, i);
		__generateHighResOctmap(LightFieldFramebuffer, LightFieldCubemap);

		LightFieldFramebuffer->set(Framebuffer::COLOR0, vioLightFieldSurface->IrradianceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR1, vioLightFieldSurface->MeanDistProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR2, vioLightFieldSurface->LowResolutionDistanceProbeGrid, CubeFace::POS_X, 0, i);
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

shared_ptr<SLightFieldSurface> App::__initLightFieldSurface()
{
	_ASSERTE(m_pProbeStatus);
	shared_ptr<SLightFieldSurface> pLightFieldSurface;
	if (m_pLightFieldSurface)
	{
		pLightFieldSurface = m_pLightFieldSurface;
		m_pLightFieldSurface->RadianceProbeGrid->clear();
		m_pLightFieldSurface->DistanceProbeGrid->clear();
		m_pLightFieldSurface->NormalProbeGrid->clear();
		m_pLightFieldSurface->IrradianceProbeGrid->clear();
		m_pLightFieldSurface->MeanDistProbeGrid->clear();
		m_pLightFieldSurface->LowResolutionDistanceProbeGrid->clear();
	}
	else
	{
		pLightFieldSurface = shared_ptr<SLightFieldSurface>(new SLightFieldSurface);
	}

	int ProbeNum = m_pProbeStatus->ProbeCounts.x * m_pProbeStatus->ProbeCounts.y * m_pProbeStatus->ProbeCounts.z;
	pLightFieldSurface->RadianceProbeGrid   = Texture::createEmpty("RaidanceProbeGrid", 1024, 1024, ImageFormat::R11G11B10F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->DistanceProbeGrid   = Texture::createEmpty("DistanceProbeGrid", 1024, 1024, ImageFormat::R16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->NormalProbeGrid     = Texture::createEmpty("NormalProbeGrid", 1024, 1024, ImageFormat::RGB16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->IrradianceProbeGrid = Texture::createEmpty("IrraidanceProbeGrid", 128, 128, ImageFormat::R11G11B10F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->MeanDistProbeGrid   = Texture::createEmpty("MeanDistProbeGrid", 128, 128, ImageFormat::RG16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->LowResolutionDistanceProbeGrid = Texture::createEmpty("LowResolutionDistanceProbeGrid", 128, 128, ImageFormat::R16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);

	return pLightFieldSurface;
}

shared_ptr<SProbeStatus> App::__initProbeStatus()
{
	shared_ptr<SProbeStatus> pProbeStatus = shared_ptr<SProbeStatus>(new SProbeStatus);
	AABox BoundingBox;
	Surface::getBoxBounds(m_posed3D, BoundingBox);
	
	pProbeStatus->BoundingBoxHeight = BoundingBox.high();
	pProbeStatus->BoundingBoxLow = BoundingBox.low();
	pProbeStatus->NegativePadding = Vector3(0.1f, 0.1f, 0.1f);
	pProbeStatus->PositivePadding = Vector3(0.1f, 0.1f, 0.1f);
	pProbeStatus->ProbeCounts = Vector3int32(2, 2, 2);
	pProbeStatus->NewProbeCounts = pProbeStatus->ProbeCounts;
	pProbeStatus->updateStatus();
	
	return pProbeStatus;
}

std::vector<Vector3> App::__placeProbe()
{
	std::vector<Vector3> ProbePositionSet;

	auto ProbeCounts   = m_pProbeStatus->ProbeCounts;
	auto ProbeSteps    = m_pProbeStatus->ProbeSteps;
	auto ProbeStartPos = m_pProbeStatus->ProbeStartPos;

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

void App::precompute()
{
	m_renderer = m_DefaultRenderer;
	m_pLightFieldSurface = __initLightFieldSurface();
	m_ProbePositionSet = __placeProbe();

	__enableEmissiveLight(false); {
		__precomputeLightFieldSurface(m_pLightFieldSurface);
	} __enableEmissiveLight(true);
	m_renderer = m_pGIRenderer;
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
