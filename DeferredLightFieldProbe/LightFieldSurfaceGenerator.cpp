#include "LightFieldSurfaceGenerator.h"
#include "App.h"
#include "HammersleySampler.h"
#include "ConfigWindow.h"

CLightFieldSurfaceGenerator::CLightFieldSurfaceGenerator(App* vApp) : m_pApp(vApp)
{
	m_pOffscreenRenderer = __initRenderer();
	m_pFramebuffer = __initFramebuffer();
	m_pGBuffer = __initGBuffer();
	m_pCamera = __initCamera();
}

shared_ptr<SLightFieldSurface> CLightFieldSurfaceGenerator::updateLightFieldSurface(shared_ptr<SLightFieldSurfaceMetaData> vMetaData)
{
	if (isNull(m_pLambertianCubeFromLight)) return nullptr;

	shared_ptr<SLightFieldSurface> LightFieldSurface = __initLightFieldSurface(vMetaData);

	Array<shared_ptr<Surface>> Surface;
	{
		Array<shared_ptr<Surface2D>> IgnoreSurface;
		m_pApp->onPose(Surface, IgnoreSurface);
	}

	shared_ptr<Texture> pSphereSamplerTexture = __createSphereSampler(vMetaData->SphereSamplerSize);
	SLightFieldCubemap LightFieldCubemap(vMetaData->ProbeCubemapResolution);
	shared_ptr<Framebuffer> pLightFieldFramebuffer = Framebuffer::create("LightFieldFramebuffer");

	//NOTE: 暂时只考虑把场景所有光源合并平均光源位置去生成LightFieldCubemap
	auto LightPosition = Vector3(0, 0, 0);
	auto EnabledLightNum = 0;
	Array<shared_ptr<Light>> LightArray = m_pApp->scene()->lightingEnvironment().lightArray;
	for (auto Light : LightArray)
	{
		if (Light->enabled())
		{
			LightPosition += Light->position().xyz();
			EnabledLightNum++;
		}
	}
	LightPosition /= EnabledLightNum;

	for (int Face = 0; Face < 6; ++Face)
	{
		__renderCubeFace(Surface, LightPosition, CubeFace(Face), Vector2int32(vMetaData->LightCubemapResolution, vMetaData->LightCubemapResolution));

		Texture::copy(m_pGBuffer->texture(GBuffer::Field::LAMBERTIAN), m_pLambertianCubeFromLight, 0, 0, 1, Vector2int16(0, 0), CubeFace::POS_X, CubeFace(Face), nullptr, false);
		Texture::copy(m_pGBuffer->texture(GBuffer::Field::WS_NORMAL), m_pWsNormalCubeFromLight, 0, 0, 1, Vector2int16(0, 0), CubeFace::POS_X, CubeFace(Face), nullptr, false);
	}

	for (int i = 0; i < vMetaData->ProbeNum(); ++i)
	{
		pLightFieldFramebuffer->set(Framebuffer::COLOR0, LightFieldSurface->IrradianceProbeGrid, CubeFace::POS_X, 0, i);
		pLightFieldFramebuffer->set(Framebuffer::COLOR1, LightFieldSurface->MeanDistProbeGrid, CubeFace::POS_X, 0, i);

		auto CubemapSampler = Sampler::cubeMap();
		CubemapSampler.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;

		m_pApp->renderDevice->push2D(pLightFieldFramebuffer); {
			m_pApp->renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
			Args args;
			args.setRect(Rect2D(Point2(0, 0), Point2(vMetaData->OctResolution, vMetaData->OctResolution)));
			args.setUniform("WsPositionFromProbe", m_PositionCubemapFromProbe[i], CubemapSampler);

			args.setUniform("LambertianFromLight", m_pLambertianCubeFromLight, CubemapSampler);
			args.setUniform("WsNormalFromLight", m_pWsNormalCubeFromLight, CubemapSampler);

			args.setUniform("WsProbePosition", vMetaData->ProbeIndexToPosition(i));
			args.setUniform("WsLightPosition", LightPosition);

			args.setUniform("OctmapResolution", vMetaData->OctResolution);
			args.setUniform("SphereSampler", pSphereSamplerTexture, Sampler::buffer());

			m_pApp->scene()->lightingEnvironment().setShaderArgs(args);

			LAUNCH_SHADER("DeferredLightFieldProbe/GenerateOctmap.pix", args);
		} m_pApp->renderDevice->pop2D();
	}

	return LightFieldSurface;
}

shared_ptr<SLightFieldSurface> CLightFieldSurfaceGenerator::generateLightFieldSurface(shared_ptr<SLightFieldSurfaceMetaData> vMetaData)
{
	m_pApp->drawMessage("Precomputing ... ...");

	shared_ptr<SLightFieldSurface> LightFieldSurface = __initLightFieldSurface(vMetaData);

	Array<shared_ptr<Surface>> Surface;
	{
		Array<shared_ptr<Surface2D>> IgnoreSurface;
		m_pApp->onPose(Surface, IgnoreSurface);
	}

	shared_ptr<Texture> pSphereSamplerTexture = __createSphereSampler(vMetaData->SphereSamplerSize);
	shared_ptr<Framebuffer> pLightFieldFramebuffer = Framebuffer::create("LightFieldFramebuffer");

	m_pLambertianCubeFromLight = Texture::createEmpty("LambertianCubeFromLight", vMetaData->LightCubemapResolution, vMetaData->LightCubemapResolution, ImageFormat::RGB8(), Texture::DIM_CUBE_MAP, false);
	m_pWsNormalCubeFromLight = Texture::createEmpty("WsNormalCubeFromLight", vMetaData->LightCubemapResolution, vMetaData->LightCubemapResolution, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, false);

	//NOTE: 暂时只考虑把场景所有光源合并平均光源位置去生成LightFieldCubemap
	auto LightPosition = Vector3(0, 0, 0);
	auto EnabledLightNum = 0;
	Array<shared_ptr<Light>> LightArray = m_pApp->scene()->lightingEnvironment().lightArray;
	for (auto Light : LightArray)
	{
		if (Light->enabled())
		{
			LightPosition += Light->position().xyz();
			EnabledLightNum++;
		}
	}
	LightPosition /= EnabledLightNum;

	//m_pApp->useSimplifiedScene(1);
	for (int Face = 0; Face < 6; ++Face)
	{
		__renderCubeFace(Surface, LightPosition, CubeFace(Face), Vector2int32(vMetaData->LightCubemapResolution, vMetaData->LightCubemapResolution));

		Texture::copy(m_pGBuffer->texture(GBuffer::Field::LAMBERTIAN), m_pLambertianCubeFromLight, 0, 0, 1, Vector2int16(0, 0), CubeFace::POS_X, CubeFace(Face), nullptr, false);
		Texture::copy(m_pGBuffer->texture(GBuffer::Field::WS_NORMAL), m_pWsNormalCubeFromLight, 0, 0, 1, Vector2int16(0, 0), CubeFace::POS_X, CubeFace(Face), nullptr, false);
	}

	//m_pApp->useSimplifiedScene(m_pApp->m_pConfigWindow->getLodLevel());
	//Surface.clear();
	//{
	//	Array<shared_ptr<Surface2D>> IgnoreSurface;
	//	m_pApp->onPose(Surface, IgnoreSurface);
	//}
	for (int i = 0; i < vMetaData->ProbeNum(); ++i)
	{
		SLightFieldCubemap LightFieldCubemap(vMetaData->ProbeCubemapResolution);
		m_PositionCubemapFromProbe.emplace_back(LightFieldCubemap.PositionCubemap);

		__renderLightFieldProbe2Cubemap(Surface, vMetaData->ProbeIndexToPosition(i), LightFieldCubemap, Vector2int32(vMetaData->ProbeCubemapResolution, vMetaData->ProbeCubemapResolution));

		pLightFieldFramebuffer->set(Framebuffer::COLOR0, LightFieldSurface->IrradianceProbeGrid, CubeFace::POS_X, 0, i);
		pLightFieldFramebuffer->set(Framebuffer::COLOR1, LightFieldSurface->MeanDistProbeGrid, CubeFace::POS_X, 0, i);

		auto CubemapSampler = Sampler::cubeMap();
		CubemapSampler.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;

		m_pApp->renderDevice->push2D(pLightFieldFramebuffer); {
			m_pApp->renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
			Args args;
			args.setRect(Rect2D(Point2(0, 0), Point2(vMetaData->OctResolution, vMetaData->OctResolution)));
			args.setUniform("WsPositionFromProbe", LightFieldCubemap.PositionCubemap, CubemapSampler);

			args.setUniform("LambertianFromLight", m_pLambertianCubeFromLight, CubemapSampler);
			args.setUniform("WsNormalFromLight", m_pWsNormalCubeFromLight, CubemapSampler);

			args.setUniform("WsProbePosition", vMetaData->ProbeIndexToPosition(i));
			args.setUniform("WsLightPosition", LightPosition);

			args.setUniform("OctmapResolution", vMetaData->OctResolution);
			args.setUniform("SphereSampler", pSphereSamplerTexture, Sampler::buffer());

			m_pApp->scene()->lightingEnvironment().setShaderArgs(args);

			LAUNCH_SHADER("DeferredLightFieldProbe/GenerateOctmap.pix", args);
		} m_pApp->renderDevice->pop2D();
	}
	//m_pApp->useSimplifiedScene(1);
	return LightFieldSurface;
}

shared_ptr<Camera> CLightFieldSurfaceGenerator::__initCamera()
{
	shared_ptr<Camera> pCamera = Camera::create("Precompute Camera");
	pCamera->setTrack(nullptr);
	pCamera->depthOfFieldSettings().setEnabled(false);
	pCamera->motionBlurSettings().setEnabled(false);
	pCamera->setFieldOfView(2.0f * ::atan(1.0f), FOVDirection::HORIZONTAL);
	pCamera->setNearPlaneZ(-0.001f);
	pCamera->setFarPlaneZ(-1000.0f);

	return pCamera;
}

shared_ptr<GBuffer> CLightFieldSurfaceGenerator::__initGBuffer(Vector2int32 vSize)
{
	GBuffer::Specification GBufferSpecification;
	GBufferSpecification.encoding[GBuffer::Field::CS_POSITION] = ImageFormat::RGB32F();
	GBufferSpecification.encoding[GBuffer::Field::WS_NORMAL] = ImageFormat::RGB32F();
	GBufferSpecification.encoding[GBuffer::Field::WS_POSITION] = ImageFormat::RGB32F();
	GBufferSpecification.encoding[GBuffer::Field::CS_NORMAL] = Texture::Encoding(ImageFormat::RGB10A2(), FrameName::CAMERA, 2.0f, -1.0f);
	GBufferSpecification.encoding[GBuffer::Field::DEPTH_AND_STENCIL] = ImageFormat::DEPTH32F();
	GBufferSpecification.encoding[GBuffer::Field::LAMBERTIAN] = ImageFormat::RGB8();

	auto r = GBuffer::create(GBufferSpecification, "Precompute GBuffer");
	r->resize(vSize);

	return r;
}

shared_ptr<COffscreenRenderer> CLightFieldSurfaceGenerator::__initRenderer()
{
	m_pOffscreenRenderer = dynamic_pointer_cast<COffscreenRenderer>(COffscreenRenderer::create());
	m_pOffscreenRenderer->setDeferredShading(true);
	return m_pOffscreenRenderer;
}

shared_ptr<Framebuffer> CLightFieldSurfaceGenerator::__initFramebuffer(Vector2int32 vSize)
{
	auto r = Framebuffer::create("Precompute Framebuffer");
	r->set(Framebuffer::COLOR0, Texture::createEmpty("COLOR0", vSize.x, vSize.y, ImageFormat::RGB32F()));

	return r;
}

shared_ptr<Texture> CLightFieldSurfaceGenerator::__createSphereSampler(Vector2int32 vSize)
{
	CHammersleySampler SphereSampler;
	std::vector<Vector3> UniformSampleDirectionsOnSphere;

	UniformSampleDirectionsOnSphere = SphereSampler.sampleSphereUniformly(vSize.x * vSize.y);

	return Texture::fromMemory("SphereSampler", UniformSampleDirectionsOnSphere.data(), ImageFormat::RGB32F(), vSize.x, vSize.y, 1, 1, ImageFormat::RGB32F(), Texture::DIM_2D, false);
}

shared_ptr<SLightFieldSurface> CLightFieldSurfaceGenerator::__initLightFieldSurface(shared_ptr<SLightFieldSurfaceMetaData> vMetaData)
{
	shared_ptr<SLightFieldSurface> pLightFieldSurface = std::make_shared<SLightFieldSurface>();

	int ProbeNum = vMetaData->ProbeNum();
	int OctResolution = vMetaData->OctResolution;
	pLightFieldSurface->IrradianceProbeGrid = Texture::createEmpty("IrraidanceProbeGrid", OctResolution, OctResolution, ImageFormat::R11G11B10F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->MeanDistProbeGrid = Texture::createEmpty("MeanDistProbeGrid", OctResolution, OctResolution, ImageFormat::RG16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);

	return pLightFieldSurface;
}

void CLightFieldSurfaceGenerator::__renderCubeFace(Array<shared_ptr<Surface>>& vSurfaces, Vector3 vRenderPosition, CubeFace vFace, Vector2int32 vCubemapResolution)
{
	auto RenderDevice = m_pApp->renderDevice;
	m_pFramebuffer->resize(vCubemapResolution);
	m_pGBuffer->resize(vCubemapResolution);

	CFrame Frame = CFrame::fromXYZYPRDegrees(vRenderPosition.x, vRenderPosition.y, vRenderPosition.z);
	Texture::getCubeMapRotation(vFace, Frame.rotation);

	m_pCamera->setFrame(Frame);

	RenderDevice->setProjectionAndCameraMatrix(m_pCamera->projection(), m_pCamera->frame());

	m_pGBuffer->prepare(RenderDevice, m_pCamera, 0, -(float)m_pApp->previousSimTimeStep(), Vector2int16(0, 0), Vector2int16(0, 0));

	m_pOffscreenRenderer->render(RenderDevice, m_pCamera, m_pFramebuffer, m_pApp->m_depthPeelFramebuffer, m_pApp->scene()->lightingEnvironment(), m_pGBuffer, vSurfaces);
}

void CLightFieldSurfaceGenerator::__renderLightFieldProbe2Cubemap(Array<shared_ptr<Surface>> allSurfaces, Vector3 vRenderPosition, SLightFieldCubemap& voLightFieldCubemap, Vector2int32 vCubemapResolution)
{
	for (int Face = 0; Face < 6; ++Face)
	{
		__renderCubeFace(allSurfaces, vRenderPosition, CubeFace(Face), vCubemapResolution);

		Texture::copy(m_pGBuffer->texture(GBuffer::Field::WS_POSITION), voLightFieldCubemap.PositionCubemap, 0, 0, 1, Vector2int16(0, 0), CubeFace::POS_X, CubeFace(Face), nullptr, false);
	}
}