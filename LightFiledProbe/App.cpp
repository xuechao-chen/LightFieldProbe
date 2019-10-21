#pragma once
#include "App.h"

void App::onInit()
{
	GApp::onInit();
	setFrameDuration(1.0f / 60.f);

	__makeGUI();

	m_gbufferSpecification.encoding[GBuffer::Field::CS_POSITION] = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::WS_POSITION] = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::WS_NORMAL]   = ImageFormat::RGB32F();

	logPrintf("Program initialized\n");
	//loadScene("Sponza Glossy");
	loadScene("Simple Cornell Box");
	//loadScene("Animated Hardening");
	//loadScene("G3D Simple Cornell Box (Globe)");
	//loadScene("G3D Breakfast Room (Area Lights)");
	//loadScene("G3D Sponza (Area Light)");
	logPrintf("Loaded Scene\n");

	m_LightFieldSurface = __initLightFieldSurface();
	m_ProbePositionSet  = __placeProbe(m_LightFieldSurface);

	__precomputeLightFieldSurface(m_LightFieldSurface);

	m_pGIRenderer = dynamic_pointer_cast<CProbeGIRenderer>(CProbeGIRenderer::create(m_LightFieldSurface));
	m_pGIRenderer->setDeferredShading(true);
	m_renderer = m_pGIRenderer;
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
	debugWindow->setVisible(false);
	showRenderingStats = false;
}

void App::__precomputeLightFieldSurface(SLightFieldSurface& vioLightFieldSurface)
{
	shared_ptr<Texture> SphereSamplerTexture = __createSphereSampler();
	shared_ptr<Texture> RadianceCubemap = Texture::createEmpty("RadianceCubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP);
	shared_ptr<Texture> DistanceCubemap = Texture::createEmpty("DistanceCubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP);

	shared_ptr<Framebuffer> LightFieldFramebuffer = Framebuffer::create("LightFieldFB");
	
	auto CubemapSampler = Sampler::cubeMap();
	CubemapSampler.interpolateMode = InterpolateMode::BILINEAR_NO_MIPMAP;

	for (int i = 0; i < m_ProbePositionSet.size(); ++i)
	{
		__renderLightFieldProbe(i, RadianceCubemap, DistanceCubemap);

		LightFieldFramebuffer->set(Framebuffer::COLOR0, vioLightFieldSurface.RadianceProbeGrid,   CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR1, vioLightFieldSurface.DistanceProbeGrid, CubeFace::POS_X, 0, i);

		renderDevice->push2D(LightFieldFramebuffer); {
			renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
			Args args;
			args.setRect(Rect2D(Point2(0, 0), Point2(1024, 1024)));
			args.setMacro("OCT_HIGH_RES", 1);
			args.setUniform("RadianceCubemap", RadianceCubemap, CubemapSampler);
			args.setUniform("DistanceCubemap", DistanceCubemap, CubemapSampler);
			args.setUniform("NumSamples", 2048);
			args.setUniform("LobeSize", 1.0f);
			args.setUniform("SphereSampler", SphereSamplerTexture, Sampler::buffer());

			LAUNCH_SHADER("PrecomputeLightFieldSurface.pix", args);
		} renderDevice->pop2D();

		LightFieldFramebuffer->set(Framebuffer::COLOR0, vioLightFieldSurface.IrradianceProbeGrid, CubeFace::POS_X, 0, i);
		LightFieldFramebuffer->set(Framebuffer::COLOR1, vioLightFieldSurface.MeanDistProbeGrid, CubeFace::POS_X, 0, i);

		renderDevice->push2D(LightFieldFramebuffer); {
			renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
			Args args;
			args.setRect(Rect2D(Point2(0, 0), Point2(128, 128)));
			args.setMacro("OCT_HIGH_RES", 0);
			args.setUniform("RadianceCubemap", RadianceCubemap, CubemapSampler);
			args.setUniform("DistanceCubemap", DistanceCubemap, CubemapSampler);
			args.setUniform("NumSamples", 2048);
			args.setUniform("LobeSize", 1.0f);
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
	LightFieldSurface.ProbeStartPosition = BoundingBox.low()+Bounds;

	auto ProbeNum = LightFieldSurface.ProbeCounts.x * LightFieldSurface.ProbeCounts.y * LightFieldSurface.ProbeCounts.z;

	LightFieldSurface.RadianceProbeGrid   = Texture::createEmpty("RaidanceProbeGrid", 1024, 1024, ImageFormat::R11G11B10F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	LightFieldSurface.DistanceProbeGrid   = Texture::createEmpty("DistanceProbeGrid", 1024, 1024, ImageFormat::R16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	LightFieldSurface.IrradianceProbeGrid = Texture::createEmpty("IrraidanceProbeGrid", 128, 128, ImageFormat::R11G11B10F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	LightFieldSurface.MeanDistProbeGrid   = Texture::createEmpty("MeanDistProbeGrid", 128, 128, ImageFormat::RG16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);

	return LightFieldSurface;
}

std::vector<Vector3> App::__placeProbe(const SLightFieldSurface& vLightFieldSurface)
{
	std::vector<Vector3> ProbePositionSet;

	auto ProbeCounts   = vLightFieldSurface.ProbeCounts;
	auto ProbeSteps    = vLightFieldSurface.ProbeSteps;
	auto ProbeStartPos = vLightFieldSurface.ProbeStartPosition;

	for (int i = 0; i < ProbeCounts.z; ++i)
	{
		for (int j = 0; j < ProbeCounts.y; ++j)
		{
			for (int k = 0; k < ProbeCounts.x; ++k)
			{
				ProbePositionSet.emplace_back(Vector3(
					ProbeStartPos.x + k * ProbeSteps.x,
					ProbeStartPos.y + j * ProbeSteps.y,
					ProbeStartPos.z + i * ProbeSteps.z
				));
			}
		}
	}

	return ProbePositionSet;
}

void App::__renderLightFieldProbe(uint32 vProbeIndex, shared_ptr<Texture> voRadianceCubemap, shared_ptr<Texture> voDistanceCubemap)
{
	Array<shared_ptr<Surface>> surface;
	{
		Array<shared_ptr<Surface2D> > ignore;
		onPose(surface, ignore);
	}

	//Array<shared_ptr<Entity>> SceneEntities;
	//scene()->getEntityArray(SceneEntities);

	//for (auto& Entity : SceneEntities) 
	//{
	//	if (Entity->name().find("light") == String::npos)
	//	{
	//		Entity->onPose(surface);
	//	}
	//}

	const int oldFramebufferWidth = m_osWindowHDRFramebuffer->width();
	const int oldFramebufferHeight = m_osWindowHDRFramebuffer->height();
	const Vector2int16  oldColorGuard = m_settings.hdrFramebuffer.colorGuardBandThickness;
	const Vector2int16  oldDepthGuard = m_settings.hdrFramebuffer.depthGuardBandThickness;
	const shared_ptr<Camera>& oldCamera = activeCamera();

	int resolution = 1024;

	m_settings.hdrFramebuffer.colorGuardBandThickness = Vector2int16(128, 128);
	m_settings.hdrFramebuffer.depthGuardBandThickness = Vector2int16(256, 256);
	const int fullWidth = resolution + (2 * m_settings.hdrFramebuffer.depthGuardBandThickness.x);
	m_osWindowHDRFramebuffer->resize(fullWidth, fullWidth);

	shared_ptr<Camera> camera = Camera::create("Cubemap Camera");
	camera->copyParametersFrom(activeCamera());
	auto Position = m_ProbePositionSet[vProbeIndex];
	CFrame cf = CFrame::fromXYZYPRDegrees(Position.x, Position.y, Position.z);
	camera->setFrame(cf);

	camera->setTrack(nullptr);
	camera->depthOfFieldSettings().setEnabled(false);
	camera->motionBlurSettings().setEnabled(false);
	camera->setFieldOfView(2.0f * ::atan(1.0f + 2.0f*(float(m_settings.hdrFramebuffer.depthGuardBandThickness.x) / float(resolution))), FOVDirection::HORIZONTAL);

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
		m_osWindowHDRFramebuffer->get(Framebuffer::DEPTH);
		Texture::copy(m_osWindowHDRFramebuffer->texture(0), voRadianceCubemap, 0, 0, 1,
					  Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - voRadianceCubemap->vector2Bounds()) / 2.0f),
					  CubeFace::POS_X, CubeFace(Face) , nullptr, false);

		Texture::copy(m_gbuffer->texture(GBuffer::Field::CS_POSITION), voDistanceCubemap, 0, 0, 1,
					  Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - voDistanceCubemap->vector2Bounds()) / 2.0f),
					  CubeFace::POS_X, CubeFace(Face), nullptr, false);
	}

	setActiveCamera(oldCamera);
	m_osWindowHDRFramebuffer->resize(oldFramebufferWidth, oldFramebufferHeight);
	m_settings.hdrFramebuffer.colorGuardBandThickness = oldColorGuard;
	m_settings.hdrFramebuffer.depthGuardBandThickness = oldDepthGuard;
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
