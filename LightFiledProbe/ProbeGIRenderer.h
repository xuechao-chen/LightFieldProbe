#pragma once
#include "Common.h"

class CDenoiser;

struct SRendererSettings
{
	bool Direct  = true;
	bool IndirectDiffuse = true;
	bool IndirectGlossy  = true;
	bool DisplayProbe    = true;
};

class CProbeGIRenderer : public DefaultRenderer
{
	friend class App;
	friend class CConfigWindow;

	SRendererSettings       m_Settings;
	shared_ptr<CDenoiser>   m_pDenoiser;
	shared_ptr<Framebuffer> m_pLightingFramebuffer;
	shared_ptr<SLightFieldSurface>         m_pLightFieldSurface;
	shared_ptr<SLightFieldSurfaceMetaData> m_pLightFieldSurfaceMetaData;

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

protected:
	CProbeGIRenderer();

	virtual void renderDeferredShading
		(RenderDevice*                      rd,
		const Array<shared_ptr<Surface> >&  sortedVisibleSurfaceArray,
		const shared_ptr<GBuffer>&          gbuffer,
		const LightingEnvironment&          environment) override;

};

