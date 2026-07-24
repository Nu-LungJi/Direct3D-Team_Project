#pragma once
#include "Engine_Defines.h"
#include "Handle.h"

namespace Engine
{
	class CResPixelShader;
	class CResVertexShader;
}

NS_BEGIN(Client)

class CMapPickingPass : public E::CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CMapPickingPass, E::CEngineBase)

public:
	HRESULT Initialize();
	std::optional<E::CHandle> Pick(uint32_t mouseX, uint32_t mouseY);
	static E::UPtr<CMapPickingPass> Create();

private:
	HRESULT RenderMapMeshObjectID(ID3D11DeviceContext* pDeferredContext);


private:
	ComPtr<ID3D11Texture2D> m_pPickingTexture{};
	ComPtr<ID3D11RenderTargetView> m_pPickingRTV{};

	ComPtr<ID3D11Texture2D> m_pDepthTexture;
	ComPtr<ID3D11DepthStencilView> m_pDepthDSV;

	ComPtr<ID3D11Texture2D> m_ReadbackTexture;
	ComPtr<ID3D11Buffer> m_pPickingCBuffer{};
	E::SPtr<E::CResVertexShader> m_pPickingVS{};
	E::SPtr<E::CResPixelShader> m_pPickingPS{};
	UINT m_iTargetWidth = 0;
	UINT m_iTargetHeight = 0;

	ComPtr<ID3D11RasterizerState> m_pRsState{};

private:
	ComPtr<ID3D11Device> m_pDevice {};
	ComPtr<ID3D11DeviceContext> m_pContext {};

	// Deferred Context는 렌더링 명령을 GPU에 즉시 보내지 않고, CommandList로 녹화하는 전용 Context
	// 명령을 실행하지 않고 CommandList에 기록
	ComPtr<ID3D11DeviceContext> m_pPickingContext{};

	std::vector<E::CHandle> m_PickTable;
};

NS_END

// 화면에 보이는 MapMeshObject 후보들만 골라와서 피킹후보들인 m_PickTable을 만듬.
// pickID 를 렌더타겟에 그림
// 디퍼드컨텍스트로 CommandList 그리기명령들 넣어주고 실행 후 바로 원래 m_pContext 상태 복원
