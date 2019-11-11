#pragma once
#include <G3D/G3D.h>

class App : public GApp
{
	shared_ptr<Texture> m_pRadianceCubemap;
	shared_ptr<Texture> m_pDistanceCubemap;
	shared_ptr<Texture> m_pNormalCubemap;

	shared_ptr<Texture> m_pCurrentCubemap;

	shared_ptr<Texture> m_pRadianceMapFromProbe;
	shared_ptr<Texture> m_pReconstructRadianceMapFromProbe;

	bool m_IsInit = false;

public:
	App(const GApp::Settings& settings = GApp::Settings());

	virtual void onInit() override;

	shared_ptr<Texture> generateOctmap(shared_ptr<Texture> vCubemapTexture, int vResolution);
	shared_ptr<Texture> reconstructRadianceMap(shared_ptr<Texture> vPositionCubemap, shared_ptr<Texture> vRadianceMapFromLight);

	void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>& allSurfaces) override;

private:
	void __makeGUI();
	void __renderLightFieldProbe(int vResolution , const Point3& vPosition, shared_ptr<Texture> voRadianceCubemap, shared_ptr<Texture> voDistanceCubemap, shared_ptr<Texture> voNormalCubemap);
};
