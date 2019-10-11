#include "ProbeGIRenderer.h"



CProbeGIRenderer::CProbeGIRenderer()
{
	m_CubemapFrameBuffer = Framebuffer::create("CProbeGIRenderer::m_CubemapFrameBuffer");
	m_CubemapFrameBuffer->set(Framebuffer::COLOR0, Texture::createEmpty("UP",1024,1024,ImageFormat::RGBA32F()));
	m_CubemapFrameBuffer->set(Framebuffer::COLOR1, Texture::createEmpty("DOWN",1024,1024,ImageFormat::RGBA32F()));
	m_CubemapFrameBuffer->set(Framebuffer::COLOR2, Texture::createEmpty("LEFT",1024,1024,ImageFormat::RGBA32F()));
	m_CubemapFrameBuffer->set(Framebuffer::COLOR3, Texture::createEmpty("RIGHT",1024,1024,ImageFormat::RGBA32F()));
	m_CubemapFrameBuffer->set(Framebuffer::COLOR4, Texture::createEmpty("FRONT",1024,1024,ImageFormat::RGBA32F()));
	m_CubemapFrameBuffer->set(Framebuffer::COLOR5, Texture::createEmpty("BACK",1024,1024,ImageFormat::RGBA32F()));
}

void CProbeGIRenderer::renderDeferredShading(RenderDevice* vRenderDevice, const Array<shared_ptr<Surface>>& vSortedVisibleSurfaceArray, const shared_ptr<GBuffer>& vGBuffer, const LightingEnvironment& vLightEnvironment)
{
	BEGIN_PROFILER_EVENT("CProbeGIRenderer::renderDeferredShading");

	vRenderDevice->setColorClearValue(Color3::black());
	vRenderDevice->clear(true, false, false);

	DefaultRenderer::renderDeferredShading(vRenderDevice, vSortedVisibleSurfaceArray, vGBuffer, vLightEnvironment);
	
	vRenderDevice->push2D(m_CubemapFrameBuffer); {
		vRenderDevice->setAlphaWrite(true);
		vRenderDevice->clear();

		vRenderDevice->setBlendFunc(Framebuffer::COLOR0, RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);

		Args args;

	} vRenderDevice->pop2D();
}
