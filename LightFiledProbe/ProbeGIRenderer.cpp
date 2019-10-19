#include "ProbeGIRenderer.h"


CProbeGIRenderer::CProbeGIRenderer(AABox vBoundingBox) : m_BoundingBox(vBoundingBox)
{
}

void CProbeGIRenderer::render(RenderDevice* rd, const shared_ptr<Camera>& camera, const shared_ptr<Framebuffer>& framebuffer, const shared_ptr<Framebuffer>& depthPeelFramebuffer,
							  LightingEnvironment& lightingEnvironment, const shared_ptr<GBuffer>& gbuffer, const Array<shared_ptr<Surface>>& allSurfaces) 
{
}