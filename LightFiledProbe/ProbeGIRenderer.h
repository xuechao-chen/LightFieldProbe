#pragma once
#include <G3D/G3D.h>

struct SLightFieldSurface
{
	Vector3int32        ProbeCounts;
	Point3              ProbeStartPosition;
	Vector3             ProbeSteps;
};

class CProbeGIRenderer : public DefaultRenderer
{
	friend class App;
protected:
	CProbeGIRenderer() {}
	CProbeGIRenderer(AABox vBoundingBox);

	virtual void renderDeferredShading
		(RenderDevice*                      vRenderDevice,
		const Array<shared_ptr<Surface> >&  vSortedVisibleSurfaceArray,
		const shared_ptr<GBuffer>&          vGBuffer,
		const LightingEnvironment&          vLightEnvironment) override;

public:
	static shared_ptr<Renderer> create(AABox vBoundingBox)
	{
		return createShared<CProbeGIRenderer>(vBoundingBox);
	}

private:
	void __initLightFiledSurface();
	void __placeProbe();
	void __generateCubemap();

private:
	AABox m_BoundingBox;
	SLightFieldSurface   m_LightFiledSurface;
	std::vector<Vector3> m_ProbePositionSet;
};

