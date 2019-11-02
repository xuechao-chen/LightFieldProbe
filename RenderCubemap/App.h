#pragma once
#include <G3D/G3D.h>

class App : public GApp
{
	shared_ptr<Texture> m_pRadianceCubemap;
	shared_ptr<Texture> m_pDistanceCubemap;
	shared_ptr<Texture> m_pNormalCubemap;

	shared_ptr<Texture> m_pCurrentCubemap;

public:
	App(const GApp::Settings& settings = GApp::Settings());

	virtual void onInit() override;

	void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>& allSurfaces) override;

private:
	void __makeGUI();
	void __renderLightFieldProbe(const Point3& vPosition, shared_ptr<Texture> voRadianceCubemap, shared_ptr<Texture> voDistanceCubemap, shared_ptr<Texture> voNormalCubemap);
};
