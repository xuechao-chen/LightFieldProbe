#pragma once
#include <G3D/G3D.h>

struct SLightFieldSurface
{
	/*ProbeGrid FB
	* texture(0): radiance
	*/
	shared_ptr<Framebuffer> ProbeGridFrameBuffer;
	Vector3int32            ProbeCounts;
	Point3                  ProbeStartPosition;
	Vector3                 ProbeSteps;
};

class CProbeGIRenderer : public DefaultRenderer
{
	friend class App;
protected:
	CProbeGIRenderer() {}
	CProbeGIRenderer(AABox vBoundingBox);

	virtual void CProbeGIRenderer::render
	    (RenderDevice*                      rd,
		const shared_ptr<Camera>&           camera,
		const shared_ptr<Framebuffer>&      framebuffer,
		const shared_ptr<Framebuffer>&      depthPeelFramebuffer,
		LightingEnvironment&                lightingEnvironment,
		const shared_ptr<GBuffer>&          gbuffer,
		const Array<shared_ptr<Surface>>&   allSurfaces) override;

	/*virtual void renderDeferredShading
		(RenderDevice*                      vRenderDevice,
		const Array<shared_ptr<Surface> >&  vSortedVisibleSurfaceArray,
		const shared_ptr<GBuffer>&          vGBuffer,
		const LightingEnvironment&          vLightEnvironment) override;
*/
public:
	static shared_ptr<Renderer> create(AABox vBoundingBox)
	{
		return createShared<CProbeGIRenderer>(vBoundingBox);
	}

private:
	void __initLightFiledSurface();
	void __placeProbe();
	void __generateCubemap();
	void __createSphereSampler(int vDegreeSize = 64);

private:
	AABox m_BoundingBox;
	SLightFieldSurface   m_LightFiledSurface;
	std::vector<Vector3> m_ProbePositionSet;
	shared_ptr<Texture>  m_CubemapTexure;
	//Vector4*             m_pSphericalSampleDirections;
	shared_ptr<Texture>  m_SphereSamplerTexture;
	shared_ptr<Texture>  m_ShadowMap;
};

