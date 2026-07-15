#include "pch.h"
#include "HizOcclusionCuller.h"
#include "HizBuffer.h"
#include "GameInstance.h"
#include "DbgLineRender.h"

NS_USING(Engine)

HRESULT CHizOcclusionCuller::Prepare(const CHizBuffer* pPrevHizBuffer)
{
	m_bPrepared = false;
	m_HizCpuMips.clear();

	if (pPrevHizBuffer == nullptr)
	{
		return E_FAIL;
	}

	const uint32_t mipCount = pPrevHizBuffer->GetMipCount();
	m_HizCpuMips.resize(mipCount);

	for (uint32_t mip = 0; mip < mipCount; ++mip)
	{
		auto& cpuMip = m_HizCpuMips[mip];
		if (FAILED(pPrevHizBuffer->ReadMipToCPU(mip, cpuMip.depths, cpuMip.width, cpuMip.height)))
		{
			m_HizCpuMips.clear();
			return E_FAIL;
		}
	}

	m_bPrepared = true;
	return S_OK;
}

void CHizOcclusionCuller::FrameStart()
{
	m_iHizCpuTested = 0;
	m_iHizCpuCulled = 0;
	m_iHizSelectedMipOverflow = 0;
	std::fill(std::begin(m_HizSelectedMipCounts), std::end(m_HizSelectedMipCounts), 0);
}

void CHizOcclusionCuller::UpdateGUI()
{
	ImGui::Text("Hi-Z CPU tested: %u", m_iHizCpuTested);
	ImGui::Text("Hi-Z CPU culled: %u", m_iHizCpuCulled);
	ImGui::Checkbox("Draw Occlusion Bounds", &m_bDrawOcclusionBounds);

	ImGui::Text("Hi-Z selected mip:");
	for (uint32_t i = 0; i < HIZ_DEBUG_MAX_MIPS; ++i)
	{
		if (m_HizSelectedMipCounts[i] > 0)
		{
			ImGui::Text("  mip %u: %u", i, m_HizSelectedMipCounts[i]);
		}
	}

	if (m_iHizSelectedMipOverflow > 0)
	{
		ImGui::Text("  overflow: %u", m_iHizSelectedMipOverflow);
	}
}

_float CHizOcclusionCuller::SampleHizCpuDepth(uint32_t mip, uint32_t x, uint32_t y) const
{
	if (mip >= m_HizCpuMips.size())
	{
		return 1.f;
	}

	const auto& cpuMip = m_HizCpuMips[mip];
	if (cpuMip.depths.empty() || cpuMip.width == 0 || cpuMip.height == 0)
	{
		return 1.f;
	}

	x = std::min(x, cpuMip.width - 1);
	y = std::min(y, cpuMip.height - 1);

	return cpuMip.depths[static_cast<size_t>(y) * cpuMip.width + x];
}

_bool CHizOcclusionCuller::IsOcclusionCulledCPU(const IRenderable* pRenderObject, _matrix matViewProj, const _float2& screenSize)
{
	if (!m_bPrepared || pRenderObject == nullptr || !pRenderObject->IsOcclusionCullable())
	{
		return false;
	}

	BoundingBox bounds{};
	if (!pRenderObject->GetOcclusionBounds(bounds))
	{
		return false;
	}

	XMFLOAT3 corners[BoundingBox::CORNER_COUNT]{};
	bounds.GetCorners(corners);

	const _float screenWidth = screenSize.x;
	const _float screenHeight = screenSize.y;
	if (screenWidth <= 0.f || screenHeight <= 0.f)
	{
		return false;
	}

	_float minX = std::numeric_limits<float>::max();
	_float minY = std::numeric_limits<float>::max();
	_float maxX = -std::numeric_limits<float>::max();
	_float maxY = -std::numeric_limits<float>::max();
	_float objectNearestDepth = 1.f;

	for (const auto& corner : corners)
	{
		const XMVECTOR worldPos = XMVectorSet(corner.x, corner.y, corner.z, 1.f);
		const XMVECTOR clipPos = XMVector4Transform(worldPos, matViewProj);
		const _float w = XMVectorGetW(clipPos);
		if (w <= 0.0001f)
		{
			return false;
		}

		const _float ndcX = XMVectorGetX(clipPos) / w;
		const _float ndcY = XMVectorGetY(clipPos) / w;
		const _float depth = XMVectorGetZ(clipPos) / w;
		if (depth < 0.f || depth > 1.f)
		{
			return false;
		}

		const _float screenX = (ndcX * 0.5f + 0.5f) * screenWidth;
		const _float screenY = (-ndcY * 0.5f + 0.5f) * screenHeight;

		minX = std::min(minX, screenX);
		minY = std::min(minY, screenY);
		maxX = std::max(maxX, screenX);
		maxY = std::max(maxY, screenY);
		objectNearestDepth = std::min(objectNearestDepth, depth);
	}

	if (maxX < 0.f || maxY < 0.f || minX > screenWidth || minY > screenHeight)
	{
		return false;
	}

	minX = std::clamp(minX, 0.f, screenWidth - 1.f);
	minY = std::clamp(minY, 0.f, screenHeight - 1.f);
	maxX = std::clamp(maxX, 0.f, screenWidth - 1.f);
	maxY = std::clamp(maxY, 0.f, screenHeight - 1.f);

	if (m_HizCpuMips.empty())
	{
		return false;
	}

	++m_iHizCpuTested;

	const _float rectWidth = std::max(1.f, maxX - minX);
	const _float rectHeight = std::max(1.f, maxY - minY);
	const _float rectSize = std::max(rectWidth, rectHeight);

	uint32_t selectedMip = 0;
	_float mipSize = rectSize;
	while (mipSize > 2.f && selectedMip + 1 < m_HizCpuMips.size())
	{
		mipSize *= 0.5f;
		++selectedMip;
	}

	if (selectedMip > 0)
	{
		--selectedMip;
	}

	if (selectedMip < HIZ_DEBUG_MAX_MIPS)
	{
		++m_HizSelectedMipCounts[selectedMip];
	}
	else
	{
		++m_iHizSelectedMipOverflow;
	}

	const auto& cpuMip = m_HizCpuMips[selectedMip];
	if (cpuMip.depths.empty() || cpuMip.width == 0 || cpuMip.height == 0)
	{
		return false;
	}

	const _float mipScaleX = static_cast<_float>(cpuMip.width) / screenWidth;
	const _float mipScaleY = static_cast<_float>(cpuMip.height) / screenHeight;

	const uint32_t mipMinX = std::min(static_cast<uint32_t>(std::floor(minX * mipScaleX)), cpuMip.width - 1);
	const uint32_t mipMinY = std::min(static_cast<uint32_t>(std::floor(minY * mipScaleY)), cpuMip.height - 1);
	const uint32_t mipMaxX = std::min(static_cast<uint32_t>(std::floor(maxX * mipScaleX)), cpuMip.width - 1);
	const uint32_t mipMaxY = std::min(static_cast<uint32_t>(std::floor(maxY * mipScaleY)), cpuMip.height - 1);

	const _float hizBias = 0.0005f;
	for (uint32_t y = mipMinY; y <= mipMaxY; ++y)
	{
		for (uint32_t x = mipMinX; x <= mipMaxX; ++x)
		{
			const _float hizDepth = SampleHizCpuDepth(selectedMip, x, y);
			if (hizDepth >= objectNearestDepth - hizBias)
			{
				DrawOcclusionBoundsDebug(pRenderObject, { 0.f, 1.f, 0.f, 1.f });
				return false;
			}
		}
	}

	++m_iHizCpuCulled;
	DrawOcclusionBoundsDebug(pRenderObject, { 1.f, 0.f, 0.f, 1.f });
	return true;
}

void CHizOcclusionCuller::DrawOcclusionBoundsDebug(const IRenderable* pRenderObject, const _float4& color) const
{
	if (!m_bDrawOcclusionBounds || pRenderObject == nullptr)
	{
		return;
	}

	BoundingBox bounds{};
	if (!pRenderObject->GetOcclusionBounds(bounds))
	{
		return;
	}

	auto* dbgLineRender = CGameInstance::Get().GetDbgLineRender();
	if (dbgLineRender == nullptr)
	{
		return;
	}

	XMFLOAT3 corners[BoundingBox::CORNER_COUNT]{};
	bounds.GetCorners(corners);

	static constexpr uint32_t edges[12][2] =
	{
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
		{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
	};

	for (const auto& edge : edges)
	{
		dbgLineRender->AddLine(corners[edge[0]], corners[edge[1]], color);
	}
}

UPtr<CHizOcclusionCuller> CHizOcclusionCuller::Create()
{
	return ToUPtr(new CHizOcclusionCuller{});
}
