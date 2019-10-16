#include "ProbeGIRenderer.h"

CProbeGIRenderer::CProbeGIRenderer(AABox vBoundingBox) : m_BoundingBox(vBoundingBox)
{
	__initLightFiledSurface();
	__placeProbe();
	__createSphereSampler(64);

	Texture::Specification CubemapSpec;
	//CubemapSpec.filename   = FilePath::concat(System::findDataFile("noonclouds"), "noonclouds_*.png");
	CubemapSpec.filename   = FilePath::concat(System::findDataFile("skybox"), "skybox_*.jpg");
	CubemapSpec.dimension  = Texture::DIM_CUBE_MAP;
	//CubemapSpec.preprocess = Texture::Preprocess::gamma(2.1f);
	
	m_CubemapTexure = Texture::create(CubemapSpec);
}

void CProbeGIRenderer::render(RenderDevice* rd, const shared_ptr<Camera>& camera, const shared_ptr<Framebuffer>& framebuffer, const shared_ptr<Framebuffer>& depthPeelFramebuffer,
							  LightingEnvironment& lightingEnvironment, const shared_ptr<GBuffer>& gbuffer, const Array<shared_ptr<Surface>>& allSurfaces) 
{
	rd->clear();

	//Light::renderShadowMaps();
	m_ShadowMap = lightingEnvironment.lightArray[0]->shadowMap()->depthTexture();



	rd->push2D(); {
		rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ZERO);

		Args args;
		args.setRect(rd->viewport());
		args.setUniform("ProbeColorCubemap", m_CubemapTexure, Sampler::cubeMap());
		args.setUniform("NumSamples", 2048);
		args.setUniform("LobeSize",   1.0f);
		args.setUniform("SphereSampler", m_SphereSamplerTexture, Sampler::buffer());

		LAUNCH_SHADER("ComputeProbeGrid.pix", args);

	} rd->pop2D();

	//generate cube
	//generate octmap
	//lighting

	/*for (auto& ProbePos : m_ProbePositionSet)
	{
		Draw::sphere(Sphere(ProbePos, 0.1f), rd);
	}*/
}

void CProbeGIRenderer::__initLightFiledSurface()
{
	auto BoundingBoxRange = m_BoundingBox.high() - m_BoundingBox.low();

	m_LightFiledSurface.ProbeCounts        = Vector3int32(4, 4, 4);//TODO
	m_LightFiledSurface.ProbeSteps         = BoundingBoxRange / m_LightFiledSurface.ProbeCounts;
	m_LightFiledSurface.ProbeStartPosition = m_BoundingBox.low();
}

void CProbeGIRenderer::__placeProbe()
{
	auto ProbeCounts   = m_LightFiledSurface.ProbeCounts;
	auto ProbeSteps    = m_LightFiledSurface.ProbeSteps;
	auto ProbeStartPos = m_LightFiledSurface.ProbeStartPosition;

	for (int i = 0; i < ProbeCounts.z; ++i)
	{
		for (int j = 0; j < ProbeCounts.y; ++j)
		{
			for (int k = 0; k < ProbeCounts.x; ++k)
			{
				m_ProbePositionSet.emplace_back(Vector3(
					ProbeStartPos.x + k * ProbeSteps.x,
					ProbeStartPos.y + j * ProbeSteps.y,
					ProbeStartPos.z + i * ProbeSteps.z
				));
			}
		}
	}
}

void CProbeGIRenderer::__generateCubemap()
{

}

void CProbeGIRenderer::__createSphereSampler(int vDegreeSize)
{
	std::vector<Vector4> UniformSampleDirectionsOnSphere;
	UniformSampleDirectionsOnSphere.resize(square(vDegreeSize));

	float PerDegree  = 360.0f / vDegreeSize;
	float RadianSeta = 0.0f, RadianFai = 0.0f;

	for (int i = 0; i < vDegreeSize; i++)
	{
		for (int k = 0; k < vDegreeSize; k++)
		{
			RadianSeta = toRadians(k*PerDegree*0.5f);
			RadianFai  = toRadians(i*PerDegree);
			UniformSampleDirectionsOnSphere[i*vDegreeSize + k] = Vector4(
				sin(RadianSeta)*cos(RadianFai),
				sin(RadianSeta)*sin(RadianFai),
				cos(RadianSeta),
				0.0);
		}
	}

	m_SphereSamplerTexture = Texture::fromMemory("SphereSampler", UniformSampleDirectionsOnSphere.data(), ImageFormat::RGBA32F(), vDegreeSize, vDegreeSize, 1, 1, ImageFormat::RGBA16F(), Texture::DIM_2D, false);
}
