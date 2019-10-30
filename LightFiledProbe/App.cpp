#pragma once
#include "App.h"

void App::onInit()
{
	GApp::onInit();
	setFrameDuration(1.0f / 60.f);

	m_gbufferSpecification.encoding[GBuffer::Field::CS_POSITION] = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::WS_POSITION] = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::WS_NORMAL]   = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::GLOSSY] = ImageFormat::RGBA32F();
	m_gbufferSpecification.encoding[GBuffer::Field::DEPTH_AND_STENCIL] = ImageFormat::DEPTH32F();
	m_gbufferSpecification.encoding[GBuffer::Field::CS_NORMAL] = Texture::Encoding(ImageFormat::RGB10A2(), FrameName::CAMERA, 2.0f, -1.0f);
	m_gbufferSpecification.encoding[GBuffer::Field::SS_POSITION_CHANGE] = ImageFormat::RGB32F();

	logPrintf("Program initialized\n");
	//loadScene("Simple Cornell Box");
	loadScene("Simple Cornell Box (Mirror)");
	//loadScene("Sponza (Glossy Area Lights)");
	//loadScene("Sponza (Statue Glossy)");
	logPrintf("Loaded Scene\n");

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

void App::__precomputeLightFieldSurface(SLightFieldSurface& vioLightFieldSurface)
{
	shared_ptr<Texture> SphereSamplerTexture = __createSphereSampler();
	SLightFieldCubemap LightFieldCubemap(1024);
	shared_ptr<Framebuffer> LightFieldFramebuffer = Framebuffer::create("LightFieldFB");
	
	auto CubemapSampler = Sampler::cubeMap();
	CubemapSampler.interpolateMode = InterpolateMode::BILINEAR_NO_MIPMAP;

	for (int i = 0; i < m_ProbePositionSet.size(); ++i)
	{
		__renderLightFieldProbe(i, 1024, LightFieldCubemap);

		LightFieldFramebuffer->set(Framebuffer::COLOR0, vioLightFieldSurface.RadianceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR1, vioLightFieldSurface.DistanceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR2, vioLightFieldSurface.NormalProbeGrid, CubeFace::POS_X, 0, i);

		renderDevice->push2D(LightFieldFramebuffer); {
			renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
			Args args;
			args.setRect(Rect2D(Point2(0, 0), Point2(1024, 1024)));
			args.setMacro("OCT_HIGH_RES", 1);
			args.setUniform("RadianceCubemap", LightFieldCubemap.RadianceCubemap, CubemapSampler);
			args.setUniform("DistanceCubemap", LightFieldCubemap.DistanceCubemap, CubemapSampler);
			args.setUniform("NormalCubemap", LightFieldCubemap.NormalCubemap, CubemapSampler);
			args.setUniform("NumSamples", 2048);
			args.setUniform("SphereSampler", SphereSamplerTexture, Sampler::buffer());

			LAUNCH_SHADER("PrecomputeLightFieldSurface.pix", args);
		} renderDevice->pop2D();

		LightFieldFramebuffer->set(Framebuffer::COLOR0, vioLightFieldSurface.IrradianceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR1, vioLightFieldSurface.MeanDistProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR2, vioLightFieldSurface.LowResolutionDistanceProbeGrid, CubeFace::POS_X, 0, i);

		renderDevice->push2D(LightFieldFramebuffer); {
			renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
			Args args;
			args.setRect(Rect2D(Point2(0, 0), Point2(128, 128)));
			args.setMacro("OCT_HIGH_RES", 0);
			args.setUniform("RadianceCubemap", LightFieldCubemap.RadianceCubemap, CubemapSampler);
			args.setUniform("DistanceCubemap", LightFieldCubemap.DistanceCubemap, CubemapSampler);
			args.setUniform("NumSamples", 2048);
			args.setUniform("SphereSampler", SphereSamplerTexture, Sampler::buffer());

			LAUNCH_SHADER("PrecomputeLightFieldSurface.pix", args);
		} renderDevice->pop2D();
	}
}

SLightFieldSurface App::__initLightFieldSurface()
{
	SLightFieldSurface LightFieldSurface;

	AABox BoundingBox;
	Surface::getBoxBounds(m_posed3D, BoundingBox);

	auto BoundingBoxRange = BoundingBox.high() - BoundingBox.low();

	auto Bounds = BoundingBoxRange * 0.1;//NOTE: range from 0.1 to 0.5

	LightFieldSurface.ProbeCounts = Vector3int32(2,2,2);
	LightFieldSurface.ProbeSteps = (BoundingBoxRange - Bounds * 2) / (LightFieldSurface.ProbeCounts - Vector3int32(1, 1, 1));
	LightFieldSurface.ProbeStartPosition = BoundingBox.low() + Bounds;

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

void App::__renderLightFieldProbe(uint32 vProbeIndex, int vResolution, SLightFieldCubemap& voLightFieldCubemap)
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

	m_settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(128, 128);
	m_settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(256, 256);
	const int fullWidth = vResolution + (2 * m_settings.hdrFramebuffer.depthGuardBandThickness.x);
	m_osWindowHDRFramebuffer->resize(fullWidth, fullWidth);

	shared_ptr<Camera> camera = Camera::create("Cubemap Camera");
	camera->copyParametersFrom(activeCamera());
	auto Position = m_ProbePositionSet[vProbeIndex];
	CFrame cf = CFrame::fromXYZYPRDegrees(Position.x, Position.y, Position.z);
	camera->setFrame(cf);

	camera->setTrack(nullptr);
	camera->depthOfFieldSettings().setEnabled(false);
	camera->motionBlurSettings().setEnabled(false);
	camera->setFieldOfView(2.0f * ::atan(1.0f + 2.0f*(float(m_settings.hdrFramebuffer.depthGuardBandThickness.x) / float(vResolution))), FOVDirection::HORIZONTAL);

	// Configure the base camera
	CFrame cframe = camera->frame();

	setActiveCamera(camera);
	for (int Face = 0; Face < 6; ++Face) {
		Texture::getCubeMapRotation(CubeFace(Face), cframe.rotation);
		camera->setFrame(cframe);

		renderDevice->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

		// Render every face twice to let the screen space reflection/refraction texture to stabilize
		onGraphics3D(renderDevice, surface);
		onGraphics3D(renderDevice, surface);

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

	setActiveCamera(oldCamera);
	m_osWindowHDRFramebuffer->resize(oldFramebufferWidth, oldFramebufferHeight);
	m_settings.hdrFramebuffer.colorGuardBandThickness = oldColorGuard;
	m_settings.hdrFramebuffer.depthGuardBandThickness = oldDepthGuard;
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
