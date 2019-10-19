#pragma once
#include <G3D/G3D.h>

class CProbeGIRenderer : public DefaultRenderer
{
	friend class App;
protected:
	CProbeGIRenderer() {}
	CProbeGIRenderer(AABox vBoundingBox);

	virtual void render
	(RenderDevice*                      rd,
		const shared_ptr<Camera>&           camera,
		const shared_ptr<Framebuffer>&      framebuffer,
		const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
		LightingEnvironment&                lightingEnvironment,
		const shared_ptr<GBuffer>&          gbuffer,
		const Array<shared_ptr<Surface>>&   allSurfaces) override;

public:
	static shared_ptr<Renderer> create(AABox vBoundingBox)
	{
		return createShared<CProbeGIRenderer>(vBoundingBox);
	}
};

