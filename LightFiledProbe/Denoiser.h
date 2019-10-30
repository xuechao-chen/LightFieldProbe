#pragma once
#include <G3D/G3D.h>

class CDenoiser :public ReferenceCountedObject
{
	shared_ptr<Texture> m_pAccumulateTexture;

	shared_ptr<Framebuffer> m_pIntermediateFramebuffer;

	TemporalFilter m_TemporalFilter;
	TemporalFilter::Settings m_TemporalFilterSettings;

	shared_ptr<BilateralFilter> m_pBilateralFilter;
	BilateralFilterSettings m_BilateralFilterSettings;

public:
	CDenoiser()
	{
		m_pBilateralFilter = BilateralFilter::create();

		m_BilateralFilterSettings = BilateralFilterSettings();
		m_BilateralFilterSettings.radius = 3;
		m_BilateralFilterSettings.stepSize = 1;

		m_TemporalFilter = TemporalFilter();

		m_TemporalFilterSettings = TemporalFilter::Settings();
		m_TemporalFilterSettings.hysteresis = 0.75;

		m_pIntermediateFramebuffer = Framebuffer::create("IntermediateFramebuffer");
	}

	static shared_ptr<CDenoiser> create()
	{
		return createShared<CDenoiser>();
	}

	void apply(RenderDevice* vRenderDevice,
		const shared_ptr<Texture>& vSource,
		const shared_ptr<Texture>& voDestination,
		const shared_ptr<GBuffer>& vGBuffer)
	{
		if (isNull(m_pAccumulateTexture))
		{
			m_pAccumulateTexture = Texture::createEmpty("AccumulateTexture", vSource->width(), vSource->height(), vSource->encoding());
		}

		if (m_pAccumulateTexture->width() != vSource->width() || m_pAccumulateTexture->height() != vSource->height())
			m_pAccumulateTexture->resize(vSource->width(), vSource->height());

		m_pAccumulateTexture = m_TemporalFilter.apply(vRenderDevice, GApp::current()->activeCamera(), vSource, vGBuffer->texture(GBuffer::Field::DEPTH_AND_STENCIL), vGBuffer->texture(GBuffer::Field::SS_POSITION_CHANGE), Vector2(0, 0), 3, m_TemporalFilterSettings);

		voDestination->clear();
		m_pIntermediateFramebuffer->set(Framebuffer::COLOR0, voDestination);

		m_pBilateralFilter->apply(vRenderDevice, m_pAccumulateTexture, m_pIntermediateFramebuffer, vGBuffer, m_BilateralFilterSettings);
	}
};