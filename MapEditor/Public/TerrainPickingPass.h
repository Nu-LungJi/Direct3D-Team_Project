#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CTerrain;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

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
	struct READBACK_SLOT
	{
		ComPtr<ID3D11Texture2D> texture{};
		bool pending = false;
	};

	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
	ComPtr<ID3D11DeviceContext> m_pPickingContext{};
	ComPtr<ID3D11Texture2D> m_pPositionTexture{};
	ComPtr<ID3D11RenderTargetView> m_pPositionRTV{};
	ComPtr<ID3D11Texture2D> m_pDepthTexture{};
	ComPtr<ID3D11DepthStencilView> m_pDepthDSV{};
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
