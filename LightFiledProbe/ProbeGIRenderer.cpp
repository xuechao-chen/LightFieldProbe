#include "ProbeGIRenderer.h"

CProbeGIRenderer::CProbeGIRenderer(AABox vBoundingBox) : m_BoundingBox(vBoundingBox)
{
	__initLightFiledSurface();
	__placeProbe();
}

void CProbeGIRenderer::render(RenderDevice* rd, const shared_ptr<Camera>& camera, const shared_ptr<Framebuffer>& framebuffer, const shared_ptr<Framebuffer>& depthPeelFramebuffer,
							  LightingEnvironment& lightingEnvironment, const shared_ptr<GBuffer>& gbuffer, const Array<shared_ptr<Surface>>& allSurfaces) 
{
	rd->clear();

	for (auto& ProbePos : m_ProbePositionSet)
	{
		Draw::sphere(Sphere(ProbePos, 0.1f), rd);
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
