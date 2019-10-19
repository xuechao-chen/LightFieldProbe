#pragma once
#include <G3D/G3D.h>
#include "App.h"

void App::onInit()
{
	GApp::onInit();
	setFrameDuration(1.0f / 60.f);

	__makeGUI();
	__createSphereSampler(64);
	m_pCubemap = Texture::createEmpty("Cubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP);

	m_gbufferSpecification.encoding[GBuffer::Field::LAMBERTIAN] = ImageFormat::RGB32F();
	//m_gbufferSpecification.encoding[GBuffer::Field::CS_NORMAL] = ImageFormat::RGB32F();
	m_gbufferSpecification.encoding[GBuffer::Field::CS_POSITION] = ImageFormat::RGB32F();

	logPrintf("Program initialized\n");
	//loadScene("G3D Sponza (Glossy)");
	loadScene("Animated Hardening");
	logPrintf("Loaded Scene\n");

	m_GbufferShader = Shader::fromFiles("shader/computeDistance.vrt","","","","shader/computeDistance.pix");

	AABox BoundingBox;
	Surface::getBoxBounds(m_posed3D, BoundingBox);
	__initLightFiledSurface(BoundingBox);
	__placeProbe();

	m_LightFiledFB = Framebuffer::create("LightFiledFB");
	//GBuffer::Specification GBufferSpec;
	//GBufferSpec.encoding[GBuffer::Field::CS_POSITION] = ImageFormat::RGB32F();

	//m_ProbeGBuffer = GBuffer::create(GBufferSpec, "ProbeGBuffer");
	//m_ProbeGBuffer->resize(Vector2int32(1024, 1024));


	//const Vector2int32 framebufferSize = m_settings.hdrFramebuffer.hdrFramebufferSizeFromDeviceSize(Vector2int32(m_deviceFramebuffer->vector2Bounds()));
	//m_framebuffer->resize(framebufferSize);
	//m_gbuffer->resize(framebufferSize);

	for (int i = 0; i < 2; ++i)
	{
		Array<shared_ptr<Texture>> CubemapTexture;

		for (int Face = 0; Face < 6; ++Face)
		{
			CubemapTexture.push(Texture::createEmpty("Cubemap" + Face, 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_2D));
		}

		__renderLightFiledProbe(40+i, CubemapTexture, CubemapTexture);

		for (int Face = 0; Face < 6; ++Face)
		{
			Texture::copy(CubemapTexture[Face], m_pCubemap, 0, 0, 1.0f, Vector2int16(0, 0), CubeFace::POS_X, static_cast<CubeFace>(Face));
		}

		m_LightFiledFB->set(Framebuffer::COLOR0, m_LightFiledSurface.RadianceProbeGrid, CubeFace::POS_X, 0, i);

		renderDevice->push2D(m_LightFiledFB); {
			renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
			auto Sampler = Sampler::cubeMap();
			Sampler.interpolateMode = InterpolateMode::BILINEAR_NO_MIPMAP;

			Args args;
			args.setRect(renderDevice->viewport());
			args.setUniform("ProbeColorCubemap", m_pCubemap, Sampler);
			args.setUniform("NumSamples", 2048);
			args.setUniform("LobeSize", 1.0f);
			args.setUniform("SphereSampler", m_SphereSamplerTexture, Sampler::buffer());

			LAUNCH_SHADER("ComputeProbeGrid.pix", args);
		} renderDevice->pop2D();

	}

	m_IsInit = true;
}

void App::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface)
{
	if (!m_IsInit) { GApp::onGraphics3D(rd, surface); return; }

	rd->swapBuffers();
	rd->clear();

	rd->push2D();
	{
		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		//auto Sampler = Sampler::buffer();
		//Sampler.interpolateMode = InterpolateMode::BILINEAR_NO_MIPMAP;

		Args args;
		args.setRect(rd->viewport());
		args.setUniform("Gbuffer", m_gbuffer->texture(GBuffer::Field::CS_POSITION), Sampler::buffer());
		//m_gbuffer->setShaderArgsRead(args, "gbuffer_");
		LAUNCH_SHADER("cubemap.pix", args);
	} rd->pop2D();

	//rd->push2D(); {
	//	rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
	//	auto Sampler = Sampler::cubeMap();
	//	Sampler.interpolateMode = InterpolateMode::BILINEAR_NO_MIPMAP;

	//	Args args;
	//	args.setRect(rd->viewport());
	//	args.setUniform("ProbeColorCubemap", m_pCubemap, Sampler);
	//	args.setUniform("NumSamples", 2048);
	//	args.setUniform("LobeSize", 1.0f);
	//	args.setUniform("SphereSampler", m_SphereSamplerTexture, Sampler::buffer());

	//	LAUNCH_SHADER("ComputeProbeGrid.pix", args);

	//} rd->pop2D();

	//GApp::onGraphics3D(rd, surface);
	//for (auto i = 0; i < m_ProbePositionSet.size(); ++i)
	//{
	//	Draw::sphere(Sphere(m_ProbePositionSet[i], 0.1), rd);
	//}
}

void App::__createSphereSampler(int vDegreeSize)
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

	m_SphereSamplerTexture = Texture::fromMemory("SphereSampler", UniformSampleDirectionsOnSphere.data(), ImageFormat::RGBA32F(), vDegreeSize, vDegreeSize, 1, 1, ImageFormat::RGBA16F(), Texture::DIM_2D, false);
}


void App::__makeGUI()
{
	debugWindow->setVisible(false);
	showRenderingStats = false;
}

void App::__initLightFiledSurface(const AABox& vBoundingBox)
{
	auto BoundingBoxRange = vBoundingBox.high() - vBoundingBox.low();

	m_LightFiledSurface.ProbeCounts = Vector3int32(4, 4, 4);//TODO
	m_LightFiledSurface.ProbeSteps = BoundingBoxRange / m_LightFiledSurface.ProbeCounts;
	m_LightFiledSurface.ProbeStartPosition = vBoundingBox.low();

	m_LightFiledSurface.RadianceProbeGrid = Texture::createEmpty("RaidanceProbeGrid", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_2D_ARRAY, false, 2);
	m_LightFiledSurface.IrradianceProbeGrid = Texture::createEmpty("IrraidanceProbeGrid", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP_ARRAY);
}


void App::__placeProbe()
{
	auto ProbeCounts   = m_LightFiledSurface.ProbeCounts;
	auto ProbeSteps    = m_LightFiledSurface.ProbeSteps;
	auto ProbeStartPos = m_LightFiledSurface.ProbeStartPosition;

	for (int i = 0; i < ProbeCounts.z; ++i)
	{
		for (int j = 0; j < ProbeCounts.y; ++j)
		{
			for (int k = 0; k < ProbeCounts.x; ++k)
			{
				m_ProbePositionSet.emplace_back(Vector3(
					ProbeStartPos.x + k * ProbeSteps.x,
					ProbeStartPos.y + j * ProbeSteps.y,
					ProbeStartPos.z + i * ProbeSteps.z
				));
			}
		}
	}
}

void App::__computeGBuffers(RenderDevice* rd, Array<shared_ptr<Surface> >& all) {
	BEGIN_PROFILER_EVENT("App::computeGBuffers");

	m_gbuffer->prepare(rd, activeCamera(), 0, -(float)previousSimTimeStep(), m_settings.hdrFramebuffer.depthGuardBandThickness, m_settings.hdrFramebuffer.colorGuardBandThickness);

	Array<shared_ptr<Surface> > sortedVisible;
	Surface::cull(activeCamera()->frame(), activeCamera()->projection(), rd->viewport(), all, sortedVisible);
	Surface::sortFrontToBack(sortedVisible, activeCamera()->frame().lookVector());
	glDisable(GL_DEPTH_CLAMP);

	Surface::renderIntoGBuffer(rd, sortedVisible, m_gbuffer);
	END_PROFILER_EVENT();
}

void App::__renderLightFiledProbe(uint32 vProbeIndex, Array<shared_ptr<Texture>>& vRadianceTexture, Array<shared_ptr<Texture>>& vDistanceTexture)
{
	Array<shared_ptr<Surface> > surface;
	{
		Array<shared_ptr<Surface2D> > ignore;
		onPose(surface, ignore);
	}

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
	CFrame cf = CFrame::fromXYZYPRDegrees(Position.x,Position.y,Position.z);
	camera->setFrame(cf);

	camera->setTrack(nullptr);
	camera->depthOfFieldSettings().setEnabled(false);
	camera->motionBlurSettings().setEnabled(false);
	camera->setFieldOfView(2.0f * ::atan(1.0f + 2.0f*(float(m_settings.hdrFramebuffer.depthGuardBandThickness.x) / float(resolution))), FOVDirection::HORIZONTAL);

	// Configure the base camera
	CFrame cframe = camera->frame();

	setActiveCamera(camera);
	for (int face = 0; face < 6; ++face) {
		Texture::getCubeMapRotation(CubeFace(face), cframe.rotation);
		camera->setFrame(cframe);

		renderDevice->setProjectionAndCameraMatrix(activeCamera()->projection(), activeCamera()->frame());

		// Render every face twice to let the screen space reflection/refraction texture to stabilize
		onGraphics3D(renderDevice, surface);
		onGraphics3D(renderDevice, surface);
		m_osWindowHDRFramebuffer->get(Framebuffer::DEPTH);
		Texture::copy(m_osWindowHDRFramebuffer->texture(0), vRadianceTexture[face], 0, 0, 1,
			Vector2int16((m_osWindowHDRFramebuffer->texture(0)->vector2Bounds() - vRadianceTexture[face]->vector2Bounds()) / 2.0f),
			CubeFace::POS_X, CubeFace::POS_X, nullptr, false);


		//Gbuffer
		//m_ProbeGBuffer->prepare(renderDevice, camera, 0, -(float)previousRealTimeStep(), Vector2int16(0, 0), Vector2int16(0, 0));
		//m_ProbeGBuffer->setCamera(camera);
		//m_gbuffer->prepare(renderDevice, camera, 0, -(float)previousSimTimeStep(), m_settings.hdrFramebuffer.depthGuardBandThickness, m_settings.hdrFramebuffer.colorGuardBandThickness);
		
		//__computeGBuffers(renderDevice, surface);

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
