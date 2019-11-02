#include "ProbeGIRenderer.h"
#include "Denoiser.h"

CProbeGIRenderer::CProbeGIRenderer(const SLightFieldSurface& vLightFieldSurface) : m_LightFieldSurface(vLightFieldSurface)
{
	auto Width  = GApp::current()->settings().window.width;
	auto Height = GApp::current()->settings().window.height;

	m_pLightingFramebuffer = Framebuffer::create("LightingFramebuffer");
	m_pLightingFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("Diffuse", Width, Height, ImageFormat::R11G11B10F()));
	m_pLightingFramebuffer->set(Framebuffer::COLOR1, Texture::createEmpty("Glossy", Width, Height, ImageFormat::R11G11B10F()));
	
	m_pDenoiser = CDenoiser::create();
	
	m_pFilteredGlossyTexture = Texture::createEmpty("FilterredGlossyTexture", Width, Height, ImageFormat::R11G11B10F());
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

	rd->push2D(m_pLightingFramebuffer); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		
		Args args;
		args.setRect(rd->viewport());

		args.setUniform("seed", rand());
		args.setUniform("numofSamples", 1);
		args.setUniform("lightFieldSurface.probeStep",		    m_LightFieldSurface.ProbeSteps);
		args.setUniform("lightFieldSurface.probeCounts",        m_LightFieldSurface.ProbeCounts);
		args.setUniform("lightFieldSurface.probeStartPosition", m_LightFieldSurface.ProbeStartPosition);

		args.setMacro("ENABLE_INDIRECT_DIFFUSE", m_Settings.IndirectDiffuse);
		args.setMacro("ENABLE_INDIRECT_GLOSSY", m_Settings.IndirectGlossy);

		gbuffer->setShaderArgsRead(args, "gbuffer_");
		m_LightFieldSurface.RadianceProbeGrid->setShaderArgs(args,   "lightFieldSurface.radianceProbeGrid.",   Sampler::buffer());
		m_LightFieldSurface.DistanceProbeGrid->setShaderArgs(args,   "lightFieldSurface.distanceProbeGrid.",   Sampler::buffer());
		m_LightFieldSurface.NormalProbeGrid->setShaderArgs(args,     "lightFieldSurface.normalProbeGrid.",     Sampler::buffer());
		m_LightFieldSurface.IrradianceProbeGrid->setShaderArgs(args, "lightFieldSurface.irradianceProbeGrid.", Sampler::buffer());
		m_LightFieldSurface.MeanDistProbeGrid->setShaderArgs(args,   "lightFieldSurface.meanDistProbeGrid.",   Sampler::buffer());
		m_LightFieldSurface.LowResolutionDistanceProbeGrid->setShaderArgs(args, "lightFieldSurface.lowResolutionDistanceProbeGrid.", Sampler::buffer());

		LAUNCH_SHADER("shader/Lighting.pix", args);

	} rd->pop2D();

	m_pDenoiser->apply(rd, m_pLightingFramebuffer->texture(1), m_pFilteredGlossyTexture, gbuffer);

	rd->push2D(); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, DstBlendFunc);
		Args args;
		args.setUniform("IndirectDiffuseTexture", m_pLightingFramebuffer->texture(0), Sampler::buffer());
		args.setUniform("IndirectGlossyTexture", m_pFilteredGlossyTexture, Sampler::buffer());
		args.setRect(rd->viewport());
		LAUNCH_SHADER("shader/Merge.pix", args);

	} rd->pop2D();

	if (m_Settings.DisplayProbe) __displayProbes(rd);
}

void CProbeGIRenderer::__displayProbes(RenderDevice* vRenderDevice, float vProbeRadius)
{
	auto ProbeCounts   = m_LightFieldSurface.ProbeCounts;
	auto ProbeSteps    = m_LightFieldSurface.ProbeSteps;
	auto ProbeStartPos = m_LightFieldSurface.ProbeStartPosition;

	//const Color4 ProbeColor = Color4(Color3::yellow(), 1);

	if (vProbeRadius <= 0) vProbeRadius = ProbeSteps.min() * 0.05f;
	
	for (int z = 0; z < ProbeCounts.z; ++z)
	{
		for (int y = 0; y < ProbeCounts.y; ++y)
		{
			for (int x = 0; x < ProbeCounts.x; ++x)
			{
				Color4 ProbeColor = Color4(x * 1.0f / ProbeCounts.x, y * 1.0f / ProbeCounts.z, z * 1.0f / ProbeCounts.z, 1.0f);
				auto ProbePos = ProbeStartPos + Vector3(x, y, z) * ProbeSteps;
				Draw::sphere(Sphere(ProbePos, vProbeRadius), vRenderDevice, ProbeColor, ProbeColor);
			}
		}
	}
}