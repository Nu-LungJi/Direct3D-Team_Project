#pragma once
#include "Engine_Defines.h"
#include "IRenderable.h"

NS_BEGIN(Engine)

class CHizBuffer;

class ENGINE_DLL CHizOcclusionCuller final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CHizOcclusionCuller, CEngineBase)

private:
	CHizOcclusionCuller() = default;
	~CHizOcclusionCuller() override = default;

public:
	// prev Hi-Z의 모든 mip의 깊이값을 CPU vector로 캐싱한다 // 임시 테스트용
	HRESULT Prepare(const CHizBuffer* pPrevHizBuffer);
	void FrameStart();
	void UpdateGUI();

	// 렌더러블 오브젝트 오클루전 컬링 검사 (CPU로 검사)
	_bool IsOcclusionCulledCPU(const IRenderable* pRenderObject, _matrix matViewProj, const _float2& screenSize);

private:
	// mip에서 depth값을 가져옴(CPU)
	_float SampleHizCpuDepth(uint32_t mip, uint32_t x, uint32_t y) const;
	void DrawOcclusionBoundsDebug(const IRenderable* pRenderObject, const _float4& color) const;

private:
	struct HIZ_CPU_MIP
	{
		std::vector<float> depths{};
		uint32_t width = 0; // mip width
		uint32_t height = 0; // mip height
	};
	// mip depth 캐싱
	std::vector<HIZ_CPU_MIP> m_HizCpuMips{};
	_bool m_bPrepared = false;

	// 디버깅GUI용
	uint32_t m_iHizCpuTested = 0;
	uint32_t m_iHizCpuCulled = 0;
	_bool m_bDrawOcclusionBounds = false;

	// Mip선택이 잘 이루어지고 있는지 확인용GUI
	static constexpr uint32_t HIZ_DEBUG_MAX_MIPS = 16;
	uint32_t m_HizSelectedMipCounts[HIZ_DEBUG_MAX_MIPS] = {};
	uint32_t m_iHizSelectedMipOverflow = 0;

public:
	static UPtr<CHizOcclusionCuller> Create();
};

NS_END
