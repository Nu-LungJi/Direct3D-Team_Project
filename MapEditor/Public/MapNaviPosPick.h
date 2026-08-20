#pragma once

#include "Engine_Defines.h"
#include "Handle.h"

namespace Engine
{
	class CResPixelShader;
	class CResVertexShader;
}

NS_BEGIN(Client)

class CMapNaviPosPickPass final : public E::CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CMapNaviPosPickPass, E::CEngineBase)

public:
	HRESULT Initialize();
	std::optional<E::_float3> Pick(uint32_t iMouseX, uint32_t iMouseY);
	static E::UPtr<CMapNaviPosPickPass>Create();
private:
	HRESULT RenderMapMeshPosition(ID3D11DeviceContext* pContext);

private:
	ComPtr<ID3D11Texture2D> m_pPositionTexture{};
	ComPtr<ID3D11RenderTargetView> m_pPositionRTV{};
	ComPtr<ID3D11Texture2D> m_pDepthTexture{};
	ComPtr<ID3D11DepthStencilView> m_pDepthDSV{};
	ComPtr<ID3D11Texture2D> m_pReadbackTexture{};
	ComPtr<ID3D11Buffer> m_pPickingCBuffer{};
	E::SPtr<E::CResVertexShader> m_pPickingVS{};
	E::SPtr<E::CResPixelShader> m_pPickingPS{};
	ComPtr<ID3D11RasterizerState> m_pRasterizerState{};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
	ComPtr<ID3D11DeviceContext> m_pPickingContext{};

private:
	UINT m_iTargetWidth{};
	UINT m_iTargetHeight{};

	std::vector<E::CHandle> m_PickTable{};
};

NS_END
