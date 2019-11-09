#include "ProbeGIRenderer.h"
#include "Denoiser.h"

CProbeGIRenderer::CProbeGIRenderer()
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
	
	rd->push2D(m_pLightingFramebuffer); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);
		
		Args args;
		args.setRect(rd->viewport());

		args.setUniform("seed", rand());
		args.setUniform("numofSamples", 1);
		args.setUniform("lightFieldSurface.probeStep", m_pLightFieldSurfaceMetaData->ProbeSteps);
		args.setUniform("lightFieldSurface.probeCounts", m_pLightFieldSurfaceMetaData->ProbeCounts);
		args.setUniform("lightFieldSurface.probeStartPosition", m_pLightFieldSurfaceMetaData->ProbeStartPos);

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
}