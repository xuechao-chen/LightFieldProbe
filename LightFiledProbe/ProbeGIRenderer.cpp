#include "ProbeGIRenderer.h"

CProbeGIRenderer::CProbeGIRenderer(AABox vBoundingBox) : m_BoundingBox(vBoundingBox)
{
	__initLightFiledSurface();
	__placeProbe();
}

void CProbeGIRenderer::renderDeferredShading(RenderDevice* vRenderDevice, const Array<shared_ptr<Surface>>& vSortedVisibleSurfaceArray, const shared_ptr<GBuffer>& vGBuffer, const LightingEnvironment& vLightEnvironment)
{
	//1. generate cubemap

	//2. generate octmap

	//3. lighting
		//3.0 direct
		//3.1 diffuse
		//3.2 glossy

	//4. probe display

	DefaultRenderer::renderDeferredShading(vRenderDevice, vSortedVisibleSurfaceArray, vGBuffer, vLightEnvironment);
	
	for (auto& ProbePos : m_ProbePositionSet)
	{
		Draw::sphere(Sphere(ProbePos, 0.1f), vRenderDevice);
	}
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
