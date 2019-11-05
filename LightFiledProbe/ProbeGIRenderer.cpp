#include "ProbeGIRenderer.h"
#include "Denoiser.h"

CProbeGIRenderer::CProbeGIRenderer(const shared_ptr<SLightFieldSurface>& vLightFieldSurface, const shared_ptr<SProbeStatus>& vProbeStatus) : m_pLightFieldSurface(vLightFieldSurface), m_pProbeStatus(vProbeStatus)
{
	auto Width  = GApp::current()->settings().window.width;
	auto Height = GApp::current()->settings().window.height;

	m_pLightingFramebuffer = Framebuffer::create("LightingFramebuffer");
	m_pLightingFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("Diffuse", Width, Height, ImageFormat::RGB32F()));
	m_pLightingFramebuffer->set(Framebuffer::COLOR1, Texture::createEmpty("Glossy", Width, Height, ImageFormat::RGB32F()));
	
	m_pDenoiser = CDenoiser::create();	
}

void CProbeGIRenderer::renderDeferredShading
	(RenderDevice*                      rd,
	const Array<shared_ptr<Surface> >&  sortedVisibleSurfaceArray,
	const shared_ptr<GBuffer>&          gbuffer,
	const LightingEnvironment&          environment) 
{	
	RenderDevice::BlendFunc DstBlendFunc = RenderDevice::BLEND_ZERO;
	if (m_Settings.Direct)
	{
		DefaultRenderer::renderDeferredShading(rd, sortedVisibleSurfaceArray, gbuffer, environment);
		DstBlendFunc = RenderDevice::BLEND_ONE;
	}
	if (!m_IsPrecomputed)
	{
		if (m_Settings.DisplayProbe) __refreshProbes(rd);
		return;
	}
	
	rd->push2D(m_pLightingFramebuffer); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		
		Args args;
		args.setRect(rd->viewport());

		args.setUniform("seed", rand());
		args.setUniform("numofSamples", 1);
		args.setUniform("lightFieldSurface.probeStep",		    m_pProbeStatus->ProbeSteps);
		args.setUniform("lightFieldSurface.probeCounts",        m_pProbeStatus->ProbeCounts);
		args.setUniform("lightFieldSurface.probeStartPosition", m_pProbeStatus->ProbeStartPos);

		args.setMacro("ENABLE_INDIRECT_DIFFUSE", m_Settings.IndirectDiffuse);
		args.setMacro("ENABLE_INDIRECT_GLOSSY", m_Settings.IndirectGlossy);

		environment.setShaderArgs(args);

		gbuffer->setShaderArgsRead(args, "gbuffer_");
		m_pLightFieldSurface->RadianceProbeGrid->setShaderArgs(args,   "lightFieldSurface.radianceProbeGrid.",   Sampler::buffer());
		m_pLightFieldSurface->DistanceProbeGrid->setShaderArgs(args,   "lightFieldSurface.distanceProbeGrid.",   Sampler::buffer());
		m_pLightFieldSurface->NormalProbeGrid->setShaderArgs(args,     "lightFieldSurface.normalProbeGrid.",     Sampler::buffer());
		m_pLightFieldSurface->IrradianceProbeGrid->setShaderArgs(args, "lightFieldSurface.irradianceProbeGrid.", Sampler::buffer());
		m_pLightFieldSurface->MeanDistProbeGrid->setShaderArgs(args,   "lightFieldSurface.meanDistProbeGrid.",   Sampler::buffer());
		m_pLightFieldSurface->LowResolutionDistanceProbeGrid->setShaderArgs(args, "lightFieldSurface.lowResolutionDistanceProbeGrid.", Sampler::buffer());

		LAUNCH_SHADER("shader/Lighting.pix", args);

	} rd->pop2D();
	
	m_pDenoiser->apply(rd, m_pLightingFramebuffer->texture(1), gbuffer);

	rd->push2D(); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, DstBlendFunc);
		Args args;
		args.setUniform("IndirectDiffuseTexture", m_pLightingFramebuffer->texture(0), Sampler::buffer());
		args.setUniform("IndirectGlossyTexture",  m_pLightingFramebuffer->texture(1), Sampler::buffer());
		args.setRect(rd->viewport());
		LAUNCH_SHADER("shader/Merge.pix", args);

	} rd->pop2D();

	if (m_Settings.DisplayProbe) __refreshProbes(rd);
}

void CProbeGIRenderer::__refreshProbes(RenderDevice* vRenderDevice, float vProbeRadius)
{
	SProbeStatus ProbeStatus = m_pProbeStatus->getNewProbeStatus();
	auto ProbeCounts = ProbeStatus.ProbeCounts;
	auto ProbeSteps = ProbeStatus.ProbeSteps;
	auto ProbeStartPos = ProbeStatus.ProbeStartPos;
	
	if (vProbeRadius <= 0) vProbeRadius = ProbeSteps.min() * 0.05f;
	
	for (int z = 0; z < ProbeCounts.z; ++z)
	{
		for (int y = 0; y < ProbeCounts.y; ++y)
		{
			for (int x = 0; x < ProbeCounts.x; ++x)
			{
				Color4 ProbeColor = Color4(x * 1.0f / ProbeCounts.x, y * 1.0f / ProbeCounts.y, z * 1.0f / ProbeCounts.z, 1.0f);
				auto ProbePos = ProbeStartPos + Vector3(x, y, z) * ProbeSteps;
				Draw::sphere(Sphere(ProbePos, vProbeRadius), vRenderDevice, ProbeColor, ProbeColor);
			}
		}
	}
}