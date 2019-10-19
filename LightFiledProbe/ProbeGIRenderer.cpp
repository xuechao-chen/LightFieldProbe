#include "ProbeGIRenderer.h"

unsigned int Cubemapfaces[6] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

struct SLightFieldSurface
{
	/*ProbeGrid FB
	* texture(0): radiance
	*/

	shared_ptr<Texture>     RadianceProbeGrid;
	shared_ptr<Texture>     IrradianceProbeGrid;

	Vector3int32            ProbeCounts;
	Point3                  ProbeStartPosition;
	Vector3                 ProbeSteps;
};


CProbeGIRenderer::CProbeGIRenderer(AABox vBoundingBox) : m_BoundingBox(vBoundingBox)
{
	//m_pCubeMapFramebuffer = Framebuffer::create("CubeMapFrameBuffer");
	//m_pCubeMapFramebuffer->set(Framebuffer::DEPTH, Texture::createEmpty("CubeMapFrameBufferDepth", 2048, 2048, ImageFormat::DEPTH32F()));

	//m_CubeMapColor = Texture::createEmpty("CubeMapColor", 2048, 2048, ImageFormat::RGB8(), Texture::Dimension::DIM_CUBE_MAP);
	//m_CubeMapDistance = Texture::createEmpty("CubeMapDistance", 2048, 2048, ImageFormat::RGB32F(), Texture::Dimension::DIM_CUBE_MAP);
	
	__initLightFiledSurface();
	__placeProbe();
	__createSphereSampler(64);

	m_pLightFiledFramebuffer = Framebuffer::create("LightFiledFramebuffer");
	m_pLightFiledFramebuffer->set(Framebuffer::DEPTH, Texture::createEmpty("Depth", 1024, 1024, ImageFormat::DEPTH32F()));

	Texture::Specification CubemapSpec;
	//CubemapSpec.filename   = FilePath::concat(System::findDataFile("noonclouds"), "noonclouds_*.png");
	CubemapSpec.filename   = FilePath::concat(System::findDataFile("skybox"), "skybox_*.jpg");
	CubemapSpec.dimension  = Texture::DIM_CUBE_MAP;
	//CubemapSpec.preprocess = Texture::Preprocess::gamma(2.1f);
	
	m_pCubemap = Texture::create(CubemapSpec);

	m_pCubemap = Texture::createEmpty("Cubemap", 1024, 1024, ImageFormat::RGB32F(), Texture::DIM_CUBE_MAP, true);
	

	//m_Shadowmap = ShadowMap::create();

	//m_ShadowmapShader = Shader::fromFiles("shader/Shadowmap.vrt", "", "", "", "shader/Shadowmap.pix");
	//
	//m_ShadowmapFB = Framebuffer::create("ShadowmapFB");
	//m_ShadowmapFB->set(Framebuffer::DEPTH, Texture::createEmpty("Depth", 1024, 1024, ImageFormat::DEPTH32F()));
}

void CProbeGIRenderer::render(RenderDevice* rd, const shared_ptr<Camera>& camera, const shared_ptr<Framebuffer>& framebuffer, const shared_ptr<Framebuffer>& depthPeelFramebuffer,
							  LightingEnvironment& lightingEnvironment, const shared_ptr<GBuffer>& gbuffer, const Array<shared_ptr<Surface>>& allSurfaces) 
{
	//rd->swapBuffers();
	//rd->clear();
	//shared_ptr<Camera> OldCamera = camera;

	//shared_ptr<Camera> CubemapCamera = Camera::create("Cubemap Camera");
	//CubemapCamera->copyParametersFrom(camera);
	//CubemapCamera->setTrack(nullptr);
	//CubemapCamera->depthOfFieldSettings().setEnabled(false);
	//CubemapCamera->motionBlurSettings().setEnabled(false);
	//CubemapCamera->setFieldOfView(2.0f * ::atan(1.0f + 2.0f*(float(GApp::current()->settings().hdrFramebuffer.depthGuardBandThickness.x) / float(1024))), FOVDirection::HORIZONTAL);


	CFrame Frame = camera->frame();
	CFrame OldFrame = Frame;

	for (auto Face = 0; Face < 6; ++Face)
	{
		Texture::getCubeMapRotation(CubeFace(Face), Frame.rotation);
		camera->setFrame(Frame);
		rd->setProjectionAndCameraMatrix(camera->projection(), camera->frame());

		shared_ptr<Texture> Temp = Texture::createEmpty("COLOR0", 1024, 1024, ImageFormat::RGB32F());
		m_pLightFiledFramebuffer->set(Framebuffer::COLOR0, Temp);
		
		DefaultRenderer::render(rd, camera, m_pLightFiledFramebuffer, depthPeelFramebuffer, lightingEnvironment, gbuffer, allSurfaces);
		Texture::copy(Temp, m_pCubemap, 0, 0, 1.0f, Vector2int16(0, 0), CubeFace::POS_X, static_cast<CubeFace>(Face));
	}

	camera->setFrame(OldFrame);

	rd->push2D(); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		Args args;
		args.setRect(rd->viewport());
		args.setUniform("ProbeColorCubemap", m_pCubemap, Sampler::cubeMap());
		args.setUniform("NumSamples", 2048);
		args.setUniform("LobeSize",   1.0f);
		args.setUniform("SphereSampler", m_SphereSamplerTexture, Sampler::buffer());

		LAUNCH_SHADER("ComputeProbeGrid.pix", args);

	} rd->pop2D();

	//rd->push2D(); {
	//	Args args;
	//	args.setUniform("ShadowMap", m_pLightFiledFramebuffer->texture(0), Sampler::buffer());
	//	args.setRect(rd->viewport());
	//	LAUNCH_SHADER("cubemap.pix", args);
	//} rd->pop2D();


	//indirect lighting


	//rd->swapBuffers();
	//rd->clear();
	//rd->setDepthWrite(true);

	//rd->setProjectionMatrix(Matrix4::orthogonalProjection(0,1024,0,1024,0.1,100,1.0));
	//rd->setCameraToWorldMatrix(lightingEnvironment.lightArray[0]->frame());
	//rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);


	//rd->push2D(m_ShadowmapFB); {
	//	Args args;
	//	const shared_ptr<UniversalSurface>&  surface = dynamic_pointer_cast<UniversalSurface>(allSurfaces[0]);
	//	const shared_ptr<UniversalSurface::GPUGeom>& gpuGeom = surface->gpuGeom();
	//	args.setAttributeArray("inPosition", gpuGeom->vertex);
	//	args.setIndexStream(gpuGeom->index);
	//	
	//	args.setUniform("View", lightingEnvironment.lightArray[0]->frame().toMatrix4());
	//	args.setUniform("Proj", Matrix4::orthogonalProjection(0, 1024, 0, 1024, 0.1, 100, 1.0));

	//	CoordinateFrame cf;
	//	allSurfaces[0]->getCoordinateFrame(cf);
	//	rd->setObjectToWorldMatrix(cf);

	//	rd->apply(m_ShadowmapShader, args);
	//} rd->pop2D();


	/*m_Shadowmap->updateDepth(rd, lightingEnvironment.lightArray[0]->frame(), 
		Matrix4::orthogonalProjection(0, 1024, 0, 1024, 0.1, 100, 1.0),
		allSurfaces);*/

	//Light::renderShadowMaps(rd, lightingEnvironment.lightArray, allSurfaces);
	//m_Shadowmap = lightingEnvironment.lightArray[0]->shadowMap();

	// Cull and sort
	//Array<shared_ptr<Surface> > sortedVisibleSurfaces, forwardOpaqueSurfaces, forwardBlendedSurfaces;
	//cullAndSort(camera, gbuffer, framebuffer->rect2DBounds(), allSurfaces, sortedVisibleSurfaces, forwardOpaqueSurfaces, forwardBlendedSurfaces);

	//rd->clear();
	//rd->setDepthWrite(true);
	//rd->setDepthTest(RenderDevice::DepthTest::DEPTH_LEQUAL);
	//rd->setProjectionAndCameraMatrix(camera->projection(), camera->frame());
	//computeGBuffer(rd, sortedVisibleSurfaces, gbuffer, depthPeelFramebuffer, lightingEnvironment.ambientOcclusionSettings.depthPeelSeparationHint);

	//DefaultRenderer::render(rd, camera, framebuffer, depthPeelFramebuffer, lightingEnvironment, gbuffer, allSurfaces);
	
	//m_CubeMapNormal = gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL);
	//m_CubeMapNormal = depthPeelFramebuffer->texture(0);

	/*rd->pushState(framebuffer); {
		rd->setColorClearValue(Color3::white() * 0.3f);
		rd->clear();
		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		Args args;
		args.setPrimitiveType(PrimitiveType::TRIANGLES);
		rd->setDepthWrite(true);
		rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);
		rd->setProjectionAndCameraMatrix(camera->projection(), camera->frame());

		CoordinateFrame cf;
		allSurfaces[0]->getCoordinateFrame(cf);
		rd->setObjectToWorldMatrix(cf);

		const shared_ptr<UniversalSurface>&  surface = dynamic_pointer_cast<UniversalSurface>(allSurfaces[0]);
		const shared_ptr<UniversalSurface::GPUGeom>& gpuGeom = surface->gpuGeom();
		args.setAttributeArray("Position", gpuGeom->vertex);
		args.setIndexStream(gpuGeom->index);

		rd->apply(m_ShadowmapShader, args);

	} rd->popState();

	

	rd->push2D(); {
		Args args;
		args.setUniform("ShadowMap", framebuffer->get(Framebuffer::DEPTH)->texture(), Sampler::buffer());
		args.setRect(rd->viewport());
		LAUNCH_SHADER("cubemap.pix", args);
	} rd->pop2D();*/

	//rd->push2D(); {
	//	rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
	//	Args args;
	//	args.setUniform("ShadowMap", m_CubeMapNormal, Sampler::buffer());
	//	args.setRect(rd->viewport());
	//	LAUNCH_SHADER("cubemap.pix", args);
	//} rd->pop2D();

	//rd->push2D(); {
	//	rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);

	//	Args args;
	//	args.setRect(rd->viewport());
	//	args.setUniform("ProbeColorCubemap", m_CubemapTexure, Sampler::cubeMap());
	//	args.setUniform("NumSamples", 2048);
	//	args.setUniform("LobeSize",   1.0f);
	//	args.setUniform("SphereSampler", m_SphereSamplerTexture, Sampler::buffer());

	//	LAUNCH_SHADER("ComputeProbeGrid.pix", args);

	//} rd->pop2D();

	//generate cube
	//generate octmap
	//lighting
}

void CProbeGIRenderer::__initLightFiledSurface()
{
	auto BoundingBoxRange = m_BoundingBox.high() - m_BoundingBox.low();

	m_LightFiledSurface.ProbeCounts        = Vector3int32(4, 4, 4);//TODO
	m_LightFiledSurface.ProbeSteps         = BoundingBoxRange / m_LightFiledSurface.ProbeCounts;
	m_LightFiledSurface.ProbeStartPosition = m_BoundingBox.low();

	m_LightFiledSurface.RadianceProbeGrid = Texture::createEmpty("RaidanceProbeGrid", 1024, 1024, ImageFormat::RGB16F(), Texture::DIM_2D_ARRAY);
	m_LightFiledSurface.IrradianceProbeGrid = Texture::createEmpty("IrraidanceProbeGrid", 1024,1024, ImageFormat::RGB16F(), Texture::DIM_CUBE_MAP_ARRAY);

}

void CProbeGIRenderer::__placeProbe()
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

void CProbeGIRenderer::__generateCubemap()
{

}

void CProbeGIRenderer::__createSphereSampler(int vDegreeSize)
{
	std::vector<Vector4> UniformSampleDirectionsOnSphere;
	UniformSampleDirectionsOnSphere.resize(square(vDegreeSize));

	float PerDegree  = 360.0f / vDegreeSize;
	float RadianSeta = 0.0f, RadianFai = 0.0f;

	for (int i = 0; i < vDegreeSize; i++)
	{
		for (int k = 0; k < vDegreeSize; k++)
		{
			RadianSeta = toRadians(k*PerDegree*0.5f);
			RadianFai  = toRadians(i*PerDegree);
			UniformSampleDirectionsOnSphere[i*vDegreeSize + k] = Vector4(
				sin(RadianSeta)*cos(RadianFai),
				sin(RadianSeta)*sin(RadianFai),
				cos(RadianSeta),
				0.0);
		}
	}

	m_SphereSamplerTexture = Texture::fromMemory("SphereSampler", UniformSampleDirectionsOnSphere.data(), ImageFormat::RGBA32F(), vDegreeSize, vDegreeSize, 1, 1, ImageFormat::RGBA16F(), Texture::DIM_2D, false);
}
