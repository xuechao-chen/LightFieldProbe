#pragma once
#include "Common.h"

class CDenoiser;

class CProbeGIRenderer : public DefaultRenderer
{
	friend class App;
	SLightFieldSurface m_LightFieldSurface;
	shared_ptr<Framebuffer> m_pTempFramebuffer;
	shared_ptr<Texture> m_pFilteredGlossyTexture;

	shared_ptr<CDenoiser> m_pDenoiser;
protected:
	CProbeGIRenderer(const SLightFieldSurface& vLightFieldSurface);

	virtual void renderDeferredShading
	(RenderDevice*                      rd,
		const Array<shared_ptr<Surface> >&  sortedVisibleSurfaceArray,
		const shared_ptr<GBuffer>&          gbuffer,
		const LightingEnvironment&          environment) override;

public:
	static shared_ptr<Renderer> create(const SLightFieldSurface& vLightFieldSurface)
	{
		return createShared<CProbeGIRenderer>(vLightFieldSurface);
	}

private:
	void __displayProbes(RenderDevice* vRenderDevice, float vProbeRadius=-1);
};

