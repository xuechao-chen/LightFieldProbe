#pragma once
#include "Common.h"
#include "OffscreenRenderer.h"

class App;

class CLightFieldSurfaceGenerator : public ReferenceCountedObject
{
public:
	App* m_pApp;

	shared_ptr<Texture> m_pLambertianCubeFromLight;
	shared_ptr<Texture> m_pWsNormalCubeFromLight;

	shared_ptr<Camera> m_pCamera;
	shared_ptr<GBuffer> m_pGBuffer;
	shared_ptr<Framebuffer> m_pFramebuffer;
	shared_ptr<COffscreenRenderer> m_pOffscreenRenderer;

	std::vector<shared_ptr<Texture>> m_PositionCubemapFromProbe;

public:
	static shared_ptr<CLightFieldSurfaceGenerator> create(App* vApp)
	{
		return createShared<CLightFieldSurfaceGenerator>(vApp);
	}

	shared_ptr<SLightFieldSurface> generateLightFieldSurface(shared_ptr<SLightFieldSurfaceMetaData> vMetaData);
	shared_ptr<SLightFieldSurface> updateLightFieldSurface(shared_ptr<SLightFieldSurfaceMetaData> vMetaData);

protected:
	CLightFieldSurfaceGenerator(App* vApp);

private:
	shared_ptr<Camera>		__initCamera();
	shared_ptr<GBuffer>     __initGBuffer(Vector2int32 vSize = Vector2int32(1024, 1024));
	shared_ptr<Framebuffer> __initFramebuffer(Vector2int32 vSize = Vector2int32(1024, 1024));
	shared_ptr<COffscreenRenderer> __initRenderer();
	shared_ptr<Texture>		__createSphereSampler(Vector2int32 vSize = Vector2int32(64, 64));

	shared_ptr<SLightFieldSurface> __initLightFieldSurface(shared_ptr<SLightFieldSurfaceMetaData> vMetaData);
	void __renderCubeFace(Array<shared_ptr<Surface>>& allSurfaces, Vector3 vRenderPosition, CubeFace vFace, Vector2int32 vCubemapResolution);
	void __renderLightFieldProbe2Cubemap(Array<shared_ptr<Surface>> allSurfaces, Vector3 vRenderPosition, SLightFieldCubemap& voLightFieldCubemap, Vector2int32 vCubemapResolution);
};