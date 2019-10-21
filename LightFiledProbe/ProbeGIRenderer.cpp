#include "ProbeGIRenderer.h"

CProbeGIRenderer::CProbeGIRenderer(const SLightFieldSurface& vLightFieldSurface) : m_LightFieldSurface(vLightFieldSurface){}

void CProbeGIRenderer::renderDeferredShading
	(RenderDevice*                      rd,
	const Array<shared_ptr<Surface> >&  sortedVisibleSurfaceArray,
	const shared_ptr<GBuffer>&          gbuffer,
	const LightingEnvironment&          environment) 
{
	DefaultRenderer::renderDeferredShading(rd, sortedVisibleSurfaceArray, gbuffer, environment);
	
	rd->push2D(); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
		
		Args args;
		gbuffer->setShaderArgsRead(args, "gbuffer_");
		args.setRect(rd->viewport());
		args.setUniform("lightFieldSurface.probeCounts", m_LightFieldSurface.ProbeCounts);
		args.setUniform("lightFieldSurface.probeStartPosition", m_LightFieldSurface.ProbeStartPosition);
		args.setUniform("lightFieldSurface.probeStep", m_LightFieldSurface.ProbeSteps);
		args.setUniform("lightFieldSurface.irradianceProbeGrid", m_LightFieldSurface.IrradianceProbeGrid, Sampler::buffer());
		args.setUniform("lightFieldSurface.meanDistProbeGrid", m_LightFieldSurface.MeanDistProbeGrid, Sampler::buffer());

		LAUNCH_SHADER("shader/Lighting.pix", args);

	} rd->pop2D();

	//Display probes

	auto ProbeCounts = m_LightFieldSurface.ProbeCounts;
	auto ProbeSteps = m_LightFieldSurface.ProbeSteps;
	auto ProbeStartPos = m_LightFieldSurface.ProbeStartPosition;

	for (int i = 0; i < ProbeCounts.x; ++i)
	{
		for (int j = 0; j < ProbeCounts.y; ++j)
		{
			for (int k = 0; k < ProbeCounts.z; ++k)
			{
				auto ProbePos = ProbeStartPos + Vector3(i, j, k) * ProbeSteps;
				Draw::sphere(Sphere(ProbePos, 0.05), rd);
			}
		}
	}
}