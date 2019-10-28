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
		args.setUniform("lightFieldSurface.radianceProbeGrid", m_LightFieldSurface.RadianceProbeGrid, Sampler::buffer());
		args.setUniform("lightFieldSurface.distanceProbeGrid", m_LightFieldSurface.DistanceProbeGrid, Sampler::buffer());
		args.setUniform("lightFieldSurface.normalProbeGrid",   m_LightFieldSurface.NormalProbeGrid, Sampler::buffer());
		args.setUniform("lightFieldSurface.lowResolutionDistanceProbeGrid", m_LightFieldSurface.LowResolutionDistanceProbeGrid, Sampler::buffer());

		LAUNCH_SHADER("shader/Lighting.pix", args);

	} rd->pop2D();

	__displayProbes(rd);
}

void CProbeGIRenderer::__displayProbes(RenderDevice* vRenderDevice, float vProbeRadius/*=-1*/)
{
	auto ProbeCounts = m_LightFieldSurface.ProbeCounts;
	auto ProbeSteps = m_LightFieldSurface.ProbeSteps;
	auto ProbeStartPos = m_LightFieldSurface.ProbeStartPosition;

	const Color4 ProbeColor = Color4(1, 1, 0, 1);

	if (vProbeRadius <= 0) vProbeRadius = ProbeSteps.min() * 0.05;

	for (int x = 0; x < ProbeCounts.x; ++x)
	{
		for (int y = 0; y < ProbeCounts.y; ++y)
		{
			for (int z = 0; z < ProbeCounts.z; ++z)
			{
				auto ProbePos = ProbeStartPos + Vector3(x, y, z) * ProbeSteps;
				Draw::sphere(Sphere(ProbePos, vProbeRadius), vRenderDevice, ProbeColor, ProbeColor);
			}
		}
	}
}