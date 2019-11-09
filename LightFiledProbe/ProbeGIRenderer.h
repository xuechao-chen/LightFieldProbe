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
	shared_ptr<SLightFieldSurfaceMetaData> m_pLightFieldSurfaceMetaData;
	shared_ptr<CDenoiser>   m_pDenoiser;
	shared_ptr<Framebuffer> m_pLightingFramebuffer;

protected:
	CProbeGIRenderer();

	virtual void renderDeferredShading
		(RenderDevice*                      rd,
		const Array<shared_ptr<Surface> >&  sortedVisibleSurfaceArray,
		const shared_ptr<GBuffer>&          gbuffer,
		const LightingEnvironment&          environment) override;

public:
	static shared_ptr<Renderer> create()
	{
		return createShared<CProbeGIRenderer>();
	}

	void updateLightFieldSurface(const shared_ptr<SLightFieldSurface>& vLightFieldSurface, const shared_ptr<SLightFieldSurfaceMetaData>& vMetaData)
	{
		m_pLightFieldSurface = vLightFieldSurface;
		m_pLightFieldSurfaceMetaData = vMetaData;
	}
};

