#include "LightFieldSurfaceGenerator.h"
#include "App.h"
#include "HammersleySampler.h"

CLightFieldSurfaceGenerator::CLightFieldSurfaceGenerator(App* vApp) : m_pApp(vApp)
{
	m_pFramebuffer = __initFramebuffer();
	m_pGBuffer = __initGBuffer();
	m_pCamera = __initCamera();
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
	SLightFieldCubemap LightFieldCubemap(vMetaData->CubemapResolution);
	shared_ptr<Framebuffer> pLightFieldFramebuffer = Framebuffer::create("LightFieldFramebuffer");

	for (int i = 0; i < vMetaData->ProbeNum(); ++i)
	{
		__renderLightFieldProbe2Cubemap(Surface, vMetaData->ProbeIndexToPosition(i), LightFieldCubemap);

		pLightFieldFramebuffer->set(Framebuffer::COLOR0, LightFieldSurface->RadianceProbeGrid, CubeFace::POS_X, 0, i);
		pLightFieldFramebuffer->set(Framebuffer::COLOR1, LightFieldSurface->DistanceProbeGrid, CubeFace::POS_X, 0, i);
		pLightFieldFramebuffer->set(Framebuffer::COLOR2, LightFieldSurface->NormalProbeGrid, CubeFace::POS_X, 0, i);
		__generateHighResOctmap(vMetaData->OctHighResolution, pLightFieldFramebuffer, LightFieldCubemap);

		pLightFieldFramebuffer->set(Framebuffer::COLOR0, LightFieldSurface->IrradianceProbeGrid, CubeFace::POS_X, 0, i);
		pLightFieldFramebuffer->set(Framebuffer::COLOR1, LightFieldSurface->MeanDistProbeGrid, CubeFace::POS_X, 0, i);
		pLightFieldFramebuffer->set(Framebuffer::COLOR2, LightFieldSurface->LowResolutionDistanceProbeGrid, CubeFace::POS_X, 0, i);
		__generateLowResOctmap(vMetaData->OctLowResolution, pLightFieldFramebuffer, LightFieldCubemap, pSphereSamplerTexture);
	}

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
	GBufferSpecification.encoding[GBuffer::Field::CS_NORMAL] = Texture::Encoding(ImageFormat::RGB10A2(), FrameName::CAMERA, 2.0f, -1.0f);
	GBufferSpecification.encoding[GBuffer::Field::DEPTH_AND_STENCIL] = ImageFormat::DEPTH32F();

	auto r = GBuffer::create(GBufferSpecification, "Precompute GBuffer");
	r->resize(vSize);

	return r;
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
	int OctHighResolution = vMetaData->OctHighResolution;
	int OctLowResolution = vMetaData->OctLowResolution;
	pLightFieldSurface->RadianceProbeGrid = Texture::createEmpty("RaidanceProbeGrid", OctHighResolution, OctHighResolution, ImageFormat::R11G11B10F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->DistanceProbeGrid = Texture::createEmpty("DistanceProbeGrid", OctHighResolution, OctHighResolution, ImageFormat::R16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->NormalProbeGrid = Texture::createEmpty("NormalProbeGrid", OctHighResolution, OctHighResolution, ImageFormat::RG16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->IrradianceProbeGrid = Texture::createEmpty("IrraidanceProbeGrid", OctLowResolution, OctLowResolution, ImageFormat::R11G11B10F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->MeanDistProbeGrid = Texture::createEmpty("MeanDistProbeGrid", OctLowResolution, OctLowResolution, ImageFormat::RG16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);
	pLightFieldSurface->LowResolutionDistanceProbeGrid = Texture::createEmpty("LowResolutionDistanceProbeGrid", OctLowResolution, OctLowResolution, ImageFormat::R16F(), Texture::DIM_2D_ARRAY, false, ProbeNum);

	return pLightFieldSurface;
}

void CLightFieldSurfaceGenerator::__renderCubeFace(Array<shared_ptr<Surface>>& vSurfaces, Vector3 vRenderPosition, CubeFace vFace)
{
	auto RenderDevice = m_pApp->renderDevice;

	CFrame Frame = CFrame::fromXYZYPRDegrees(vRenderPosition.x, vRenderPosition.y, vRenderPosition.z);
	Texture::getCubeMapRotation(vFace, Frame.rotation);

	m_pCamera->setFrame(Frame);

	RenderDevice->setProjectionAndCameraMatrix(m_pCamera->projection(), m_pCamera->frame());

	m_pGBuffer->prepare(RenderDevice, m_pCamera, 0, -(float)m_pApp->previousSimTimeStep(), Vector2int16(0, 0), Vector2int16(0, 0));

	m_pApp->m_pDefaultRenderer->render(RenderDevice, m_pCamera, m_pFramebuffer, m_pApp->m_depthPeelFramebuffer, m_pApp->scene()->lightingEnvironment(), m_pGBuffer, vSurfaces);

	RenderDevice->pushState(m_pFramebuffer); {
		m_pApp->m_depthOfField->apply(RenderDevice, m_pFramebuffer->texture(0), m_pFramebuffer->texture(Framebuffer::DEPTH), m_pCamera, Vector2int16(0, 0));
		m_pApp->m_motionBlur->apply(RenderDevice, m_pFramebuffer->texture(0), m_pGBuffer->texture(GBuffer::Field::SS_POSITION_CHANGE), m_pFramebuffer->texture(Framebuffer::DEPTH), m_pCamera, Vector2int16(0, 0));
	} RenderDevice->popState();

	m_pApp->m_film->exposeAndRender(RenderDevice, m_pCamera->filmSettings(), m_pFramebuffer->texture(0), 0, 0,
		Texture::opaqueBlackIfNull(m_pGBuffer ? m_pGBuffer->texture(GBuffer::Field::SS_POSITION_CHANGE) : nullptr),
		m_pCamera->jitterMotion());
}

void CLightFieldSurfaceGenerator::__renderLightFieldProbe2Cubemap(Array<shared_ptr<Surface>> allSurfaces, Vector3 vRenderPosition, SLightFieldCubemap& voLightFieldCubemap)
{
	for (int Face = 0; Face < 6; ++Face)
	{
		__renderCubeFace(allSurfaces, vRenderPosition, CubeFace(Face));

		Texture::copy(m_pFramebuffer->texture(0), voLightFieldCubemap.RadianceCubemap, 0, 0, 1, Vector2int16(0, 0), CubeFace::POS_X, CubeFace(Face), nullptr, false);
		Texture::copy(m_pGBuffer->texture(GBuffer::Field::WS_NORMAL), voLightFieldCubemap.NormalCubemap, 0, 0, 1, Vector2int16(0, 0), CubeFace::POS_X, CubeFace(Face), nullptr, false);
		Texture::copy(m_pGBuffer->texture(GBuffer::Field::CS_POSITION), voLightFieldCubemap.DistanceCubemap, 0, 0, 1, Vector2int16(0, 0), CubeFace::POS_X, CubeFace(Face), nullptr, false);
	}
}

void CLightFieldSurfaceGenerator::__generateLowResOctmap(int vResolution, std::shared_ptr<Framebuffer>& vLightFieldFramebuffer, SLightFieldCubemap& vLightFieldCubemap, shared_ptr<Texture>& vSphereSamplerTexture)
{
	auto CubemapSampler = Sampler::cubeMap();
	CubemapSampler.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;

	m_pApp->renderDevice->push2D(vLightFieldFramebuffer); {
		m_pApp->renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		Args args;
		args.setRect(Rect2D(Point2(0, 0), Point2(vResolution, vResolution)));
		args.setUniform("RadianceCubemap", vLightFieldCubemap.RadianceCubemap, CubemapSampler);
		args.setUniform("DistanceCubemap", vLightFieldCubemap.DistanceCubemap, CubemapSampler);
		args.setUniform("OctmapResolution", vResolution);
		args.setUniform("SphereSampler", vSphereSamplerTexture, Sampler::buffer());

		LAUNCH_SHADER("GenerateLowResOctmap.pix", args);
	} m_pApp->renderDevice->pop2D();
}

void CLightFieldSurfaceGenerator::__generateHighResOctmap(int vResolution, std::shared_ptr<Framebuffer>& vLightFieldFramebuffer, SLightFieldCubemap& vLightFieldCubemap)
{
	auto CubemapSampler = Sampler::cubeMap();
	CubemapSampler.interpolateMode = InterpolateMode::NEAREST_NO_MIPMAP;
	
	m_pApp->renderDevice->push2D(vLightFieldFramebuffer); {
		m_pApp->renderDevice->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);

		Args args;
		args.setRect(Rect2D(Point2(0, 0), Point2(vResolution, vResolution)));
		args.setUniform("RadianceCubemap", vLightFieldCubemap.RadianceCubemap, CubemapSampler);
		args.setUniform("DistanceCubemap", vLightFieldCubemap.DistanceCubemap, CubemapSampler);
		args.setUniform("NormalCubemap", vLightFieldCubemap.NormalCubemap, CubemapSampler);
		args.setUniform("OctmapResolution", vResolution);

		LAUNCH_SHADER("GenerateHighResOctmap.pix", args);
	} m_pApp->renderDevice->pop2D();
}
