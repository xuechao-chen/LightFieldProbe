#include <G3D/G3D.h>

class COffscreenRenderer : public DefaultRenderer
{
public:
	void render
	(RenderDevice*                       rd,
		const shared_ptr<Camera>&           camera,
		const shared_ptr<Framebuffer>&      framebuffer,
		const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
		LightingEnvironment&                lightingEnvironment,
		const shared_ptr<GBuffer>&          gbuffer,
		const Array<shared_ptr<Surface>>&   allSurfaces) override
	{
		if (notNull(gbuffer)) {
			framebuffer->set(Framebuffer::DEPTH, gbuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL));
		}
		if (notNull(depthPeelFramebuffer)) {
			depthPeelFramebuffer->resize(framebuffer->width(), framebuffer->height());
		}

		// Cull and sort
		Array<shared_ptr<Surface> > sortedVisibleSurfaces, forwardOpaqueSurfaces, forwardBlendedSurfaces;
		cullAndSort(camera, gbuffer, framebuffer->rect2DBounds(), allSurfaces, sortedVisibleSurfaces, forwardOpaqueSurfaces, forwardBlendedSurfaces);

		debugAssert(framebuffer);
		// Bind the main framebuffer
		rd->pushState(framebuffer); {
			rd->clear();
			rd->setProjectionAndCameraMatrix(camera->projection(), camera->frame());

			const bool needDepthPeel = lightingEnvironment.ambientOcclusionSettings.useDepthPeelBuffer && lightingEnvironment.ambientOcclusionSettings.enabled;
			if (notNull(gbuffer)) {
				computeGBuffer(rd, sortedVisibleSurfaces, gbuffer, needDepthPeel ? depthPeelFramebuffer : nullptr, lightingEnvironment.ambientOcclusionSettings.depthPeelSeparationHint);
			}

			// Shadowing + AO
			computeShadowing(rd, allSurfaces, gbuffer, depthPeelFramebuffer, lightingEnvironment);
			debugAssertM(allSurfaces.size() < 500000, "It is very unlikely that you intended to draw 500k surfaces. There is probably heap corruption.");

			// Maybe launch deferred pass
			if (deferredShading()) {
				renderIndirectIllumination(rd, sortedVisibleSurfaces, gbuffer, lightingEnvironment);
				renderDeferredShading(rd, sortedVisibleSurfaces, gbuffer, lightingEnvironment);
			}
		} rd->popState();
	}

public:
	static shared_ptr<COffscreenRenderer> create()
	{
		return createShared<COffscreenRenderer>();
	}
};