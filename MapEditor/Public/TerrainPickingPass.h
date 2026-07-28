#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CTerrain;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

// 마우스 픽셀에 해당하는 Terrain 표면의 월드 좌표를 GPU로 계산한다

// Terrain 월드 위치를 Float Render Target에 출력한 뒤,
// 마우스 위치의 한 픽셀만 Staging Texture로 복사해 비동기로 읽는다

class CTerrainPickingPass final : public E::CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CTerrainPickingPass, E::CEngineBase)

private:
	CTerrainPickingPass() = default;
	~CTerrainPickingPass() override = default;

public:
	std::optional<E::_float3> Pick(const E::CTerrain& terrain, uint32_t mouseX, uint32_t mouseY);
	static E::UPtr<CTerrainPickingPass> Create();

private:
	HRESULT Initialize();
	HRESULT RenderTerrainPosition(const E::CTerrain& terrain, ID3D11DeviceContext* context,
		uint32_t mouseX, uint32_t mouseY);

private:
	// 스테이징 텍스쳐
	struct READBACK_SLOT
	{
		ComPtr<ID3D11Texture2D> texture{};
		bool pending = false;
	};

	ComPtr<ID3D11Device> m_pDevice{};
	// Immediate Context
	ComPtr<ID3D11DeviceContext> m_pContext{};
	// Defered Context
	ComPtr<ID3D11DeviceContext> m_pPickingContext{};

	// 렌더타겟
	ComPtr<ID3D11Texture2D> m_pPositionTexture{};
	ComPtr<ID3D11RenderTargetView> m_pPositionRTV{};

	ComPtr<ID3D11Texture2D> m_pDepthTexture{};
	ComPtr<ID3D11DepthStencilView> m_pDepthDSV{};

	// 마우스 주변만 Rasterize 하도록 제한
	ComPtr<ID3D11RasterizerState> m_pScissorRasterizerState{};

	std::array<READBACK_SLOT, 3> m_ReadbackSlots{};
	ComPtr<ID3D11Buffer> m_pPickingCBuffer{};


	E::SPtr<E::CResVertexShader> m_pPickingVS{};
	E::SPtr<E::CResPixelShader> m_pPickingPS{};

	UINT m_iTargetWidth = 0;
	UINT m_iTargetHeight = 0;
	uint32_t m_iNextReadbackSlot = 0;
	bool m_bHasReadbackResult = false;
	std::optional<E::_float3> m_LastReadbackResult{};
};

NS_END
