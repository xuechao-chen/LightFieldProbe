#pragma once
#include <G3D/G3D.h>

class CProbeGIRenderer : public DefaultRenderer
{
	friend class App;
public:
	CProbeGIRenderer();
	~CProbeGIRenderer();

protected:
	CProbeGIRenderer();

	virtual void renderDeferredShading
		(RenderDevice*                      vRenderDevice,
		const Array<shared_ptr<Surface> >&  vSortedVisibleSurfaceArray,
		const shared_ptr<GBuffer>&          vGBuffer,
		const LightingEnvironment&          vLightEnvironment) override;

public:
	static shared_ptr<Renderer> create()
	{
		return createShared<CProbeGIRenderer>();
	}
};

