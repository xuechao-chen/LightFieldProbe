#include "ProbeGIRenderer.h"

CProbeGIRenderer::CProbeGIRenderer()
{
	auto Width  = GApp::current()->settings().window.width;
	auto Height = GApp::current()->settings().window.height;

	m_pLightingFramebuffer = Framebuffer::create("LightingFramebuffer");
	m_pLightingFramebuffer->set(Framebuffer::COLOR0, Texture::createEmpty("Diffuse", Width, Height, ImageFormat::RGB32F()));
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
	
	rd->push2D(); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, DstBlendFunc);
		
		Args args;
		args.setRect(rd->viewport());

		args.setUniform("lightFieldSurface.probeStep", m_pLightFieldSurfaceMetaData->ProbeSteps);
		args.setUniform("lightFieldSurface.probeCounts", m_pLightFieldSurfaceMetaData->ProbeCounts);
		args.setUniform("lightFieldSurface.probeStartPosition", m_pLightFieldSurfaceMetaData->ProbeStartPos);

		args.setMacro("ENABLE_INDIRECT_DIFFUSE", m_Settings.IndirectDiffuse);

		environment.setShaderArgs(args);

		gbuffer->setShaderArgsRead(args, "gbuffer_");

		m_pLightFieldSurface->IrradianceProbeGrid->setShaderArgs(args, "lightFieldSurface.irradianceProbeGrid.", Sampler::buffer());
		m_pLightFieldSurface->MeanDistProbeGrid->setShaderArgs(args,   "lightFieldSurface.meanDistProbeGrid.",   Sampler::buffer());

		LAUNCH_SHADER("DeferredLightFieldProbe/Lighting.pix", args);
	} rd->pop2D();
}