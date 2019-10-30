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
		args.setRect(rd->viewport());
		args.setUniform("seed", rand());
		args.setUniform("numofSamples", 1);
		args.setUniform("lightFieldSurface.probeStep",		    m_LightFieldSurface.ProbeSteps);
		args.setUniform("lightFieldSurface.probeCounts",        m_LightFieldSurface.ProbeCounts);
		args.setUniform("lightFieldSurface.probeStartPosition", m_LightFieldSurface.ProbeStartPosition);

		gbuffer->setShaderArgsRead(args, "gbuffer_");
		m_LightFieldSurface.RadianceProbeGrid->setShaderArgs(args,   "lightFieldSurface.radianceProbeGrid.",   Sampler::buffer());
		m_LightFieldSurface.DistanceProbeGrid->setShaderArgs(args,   "lightFieldSurface.distanceProbeGrid.",   Sampler::buffer());
		m_LightFieldSurface.NormalProbeGrid->setShaderArgs(args,     "lightFieldSurface.normalProbeGrid.",     Sampler::buffer());
		m_LightFieldSurface.IrradianceProbeGrid->setShaderArgs(args, "lightFieldSurface.irradianceProbeGrid.", Sampler::buffer());
		m_LightFieldSurface.MeanDistProbeGrid->setShaderArgs(args,   "lightFieldSurface.meanDistProbeGrid.",   Sampler::buffer());
		m_LightFieldSurface.LowResolutionDistanceProbeGrid->setShaderArgs(args, "lightFieldSurface.lowResolutionDistanceProbeGrid.", Sampler::buffer());

		LAUNCH_SHADER("shader/Lighting.pix", args);

	} rd->pop2D();

	__displayProbes(rd);
}

void CProbeGIRenderer::__displayProbes(RenderDevice* vRenderDevice, float vProbeRadius/*=-1*/)
{
	auto ProbeCounts   = m_LightFieldSurface.ProbeCounts;
	auto ProbeSteps    = m_LightFieldSurface.ProbeSteps;
	auto ProbeStartPos = m_LightFieldSurface.ProbeStartPosition;

	const Color4 ProbeColor = Color4(Color3::yellow(), 1);

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