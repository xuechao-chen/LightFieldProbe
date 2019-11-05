#pragma once
#include "Common.h"

class CDenoiser;

struct SRendererSettings
{
	bool Direct  = true;
	bool IndirectDiffuse = true;
	bool IndirectGlossy  = true;
	bool DisplayProbe = false;
};

class CProbeGIRenderer : public DefaultRenderer
{
	friend class App;
	friend class CConfigWindow;

	bool m_IsPrecomputed = false;
	
	SRendererSettings       m_Settings;
	shared_ptr<SLightFieldSurface> m_pLightFieldSurface;
	shared_ptr<SProbeStatus> m_pProbeStatus;
	shared_ptr<CDenoiser>   m_pDenoiser;
	shared_ptr<Framebuffer> m_pLightingFramebuffer;

protected:
	CProbeGIRenderer(const shared_ptr<SLightFieldSurface>& vLightFieldSurface, const shared_ptr<SProbeStatus>& vProbeStatus);

	virtual void renderDeferredShading
		(RenderDevice*                      rd,
		const Array<shared_ptr<Surface> >&  sortedVisibleSurfaceArray,
		const shared_ptr<GBuffer>&          gbuffer,
		const LightingEnvironment&          environment) override;

public:
	static shared_ptr<Renderer> create(const shared_ptr<SLightFieldSurface>& vLightFieldSurface, const shared_ptr<SProbeStatus>& vProbeStatus)
	{
		return createShared<CProbeGIRenderer>(vLightFieldSurface, vProbeStatus);
	}

private:
	void __refreshProbes(RenderDevice* vRenderDevice, float vProbeRadius = -1);
};

