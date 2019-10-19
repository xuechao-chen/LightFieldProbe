#pragma once
#include <G3D/G3D.h>

struct SLightFieldSurface
{
	shared_ptr<Texture>     RadianceProbeGrid;
	shared_ptr<Texture>     IrradianceProbeGrid;

	Vector3int32            ProbeCounts;
	Point3                  ProbeStartPosition;
	Vector3                 ProbeSteps;

};

class App : public GApp
{
	shared_ptr<Shader>      m_GbufferShader;

	shared_ptr<Framebuffer> m_ProbeGBuffer;
	shared_ptr<Framebuffer> m_LightFiledFB;

	SLightFieldSurface   m_LightFiledSurface;
	std::vector<Vector3> m_ProbePositionSet;

	shared_ptr<Texture>  m_SphereSamplerTexture;
	shared_ptr<Texture>  m_pCubemap;

	bool m_IsInit = false;
public:
	App(const GApp::Settings& vSettings) : GApp(vSettings) {}

	virtual void onInit() override;
	virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface) override;


private:
	void __makeGUI();
	void __createSphereSampler(int vDegreeSize);
	void __initLightFiledSurface(const AABox& vBoundingBox);
	void __placeProbe();
	void __renderLightFiledProbe(uint32 vProbeIndex, Array<shared_ptr<Texture>>& vRadianceTexture, Array<shared_ptr<Texture>>& vDistanceTexture);
	void __computeGBuffers(RenderDevice* rd, Array<shared_ptr<Surface> >& all);
};