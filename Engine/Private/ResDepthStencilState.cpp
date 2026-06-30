#include "pch.h"
#include "ResDepthStencilState.h"
#include "GameInstance.h"
NS_USING(Engine)

CResDepthStencilState::CResDepthStencilState(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResource{ sPath }
	, m_pDevice{ pDevice }
	, m_pContext{ pContext } 
{
}

CResDepthStencilState::~CResDepthStencilState()
{
}

HRESULT CResDepthStencilState::Load(const std::any& arg)
{
	auto desc = std::any_cast<D3D11_DEPTH_STENCIL_DESC>(&arg);
	if (!desc)
	{
		return E_FAIL;
	}
	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	m_eState = STATE::LOADING;

	if (FAILED(m_pDevice->CreateDepthStencilState(desc, m_pDepthStencilState.GetAddressOf())))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResDepthStencilState::Unload(const std::any& arg)
{
    return S_OK;
}

SPtr<CResDepthStencilState> CResDepthStencilState::Create()
{
	return ToSPtr(new CResDepthStencilState{"", CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
