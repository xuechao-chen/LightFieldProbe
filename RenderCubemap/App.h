#pragma once
#include <G3D/G3D.h>

class App : public GApp
{
	shared_ptr<Texture> m_pCubemapTexture;

public:
	App(const GApp::Settings& settings = GApp::Settings());

	virtual void onInit() override;

private:
	void __renderLightFieldProbe(const Point3& vPosition, shared_ptr<Texture> voCubemapTexture);
};
