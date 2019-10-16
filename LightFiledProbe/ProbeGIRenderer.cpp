#include "ProbeGIRenderer.h"

CProbeGIRenderer::CProbeGIRenderer(AABox vBoundingBox) : m_BoundingBox(vBoundingBox)
{
	__placeProbe();
}

void CProbeGIRenderer::renderDeferredShading(RenderDevice* vRenderDevice, const Array<shared_ptr<Surface>>& vSortedVisibleSurfaceArray, const shared_ptr<GBuffer>& vGBuffer, const LightingEnvironment& vLightEnvironment)
{
	Surface::getBoxBounds(vSortedVisibleSurfaceArray, m_BoundingBox);
	//1. generate cubemap

	//2. generate octmap

	//3. lighting
		//3.0 direct
		//3.1 diffuse
		//3.2 glossy
}

void CProbeGIRenderer::__placeProbe()
{
}
