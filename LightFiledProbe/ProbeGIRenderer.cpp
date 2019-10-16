#include "ProbeGIRenderer.h"

CProbeGIRenderer::CProbeGIRenderer(AABox vBoundingBox) : m_BoundingBox(vBoundingBox)
{
	m_pCubeMapFramebuffer = Framebuffer::create("CubeMapFrameBuffer");
	m_pCubeMapFramebuffer->set(Framebuffer::DEPTH, Texture::createEmpty("CubeMapFrameBufferDepth", 2048, 2048, ImageFormat::DEPTH32F()));

	m_CubeMapColor = Texture::createEmpty("CubeMapColor", 2048, 2048, ImageFormat::RGB8(), Texture::Dimension::DIM_CUBE_MAP);
	m_CubeMapDistance = Texture::createEmpty("CubeMapDistance", 2048, 2048, ImageFormat::RGB32F(), Texture::Dimension::DIM_CUBE_MAP);
	
	__initLightFiledSurface();
	__placeProbe();
}

void CProbeGIRenderer::render(RenderDevice* rd, const shared_ptr<Camera>& camera, const shared_ptr<Framebuffer>& framebuffer, const shared_ptr<Framebuffer>& depthPeelFramebuffer,
							  LightingEnvironment& lightingEnvironment, const shared_ptr<GBuffer>& gbuffer, const Array<shared_ptr<Surface>>& allSurfaces) 
{
	//rd->swapBuffers();
	rd->clear();


	//Light::renderShadowMaps(rd, lightingEnvironment.lightArray, allSurfaces);

	//for (auto& ProbePos : m_ProbePositionSet)
	//{
	//	Draw::sphere(Sphere(ProbePos, 0.1f), rd);
	//}

	rd->push2D(); {

		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		Args args;
		args.setUniform("FRONT", lightingEnvironment.lightArray[1]->shadowMap()->depthTexture(), Sampler::buffer());
		
		args.setRect(rd->viewport());

		LAUNCH_SHADER("cubemap.*", args);

	} rd->pop2D();
}

void CProbeGIRenderer::__initLightFiledSurface()
{
	auto BoundingBoxRange = m_BoundingBox.high() - m_BoundingBox.low();

	m_LightFiledSurface.ProbeCounts        = Vector3int32(4, 4, 4);//TODO
	m_LightFiledSurface.ProbeSteps         = BoundingBoxRange / m_LightFiledSurface.ProbeCounts;
	m_LightFiledSurface.ProbeStartPosition = m_BoundingBox.low();
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
