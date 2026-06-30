#include "pch.h"
#include "ResRasterizerState.h"
#include "GameInstance.h"
NS_USING(Engine)

CResRasterizerState::CResRasterizerState(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResource{ sPath }
	, m_pDevice{ pDevice }
	, m_pContext{ pContext } 
{
}

CResRasterizerState::~CResRasterizerState()
{
}

HRESULT CResRasterizerState::Load(const std::any& arg)
{
	auto desc = std::any_cast<D3D11_RASTERIZER_DESC>(&arg);
	if (!desc)
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}
	
	{
		m_eState = STATE::LOADING;

		if (FAILED(m_pDevice->CreateRasterizerState(desc, m_pRasterizerState.GetAddressOf())))
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}
	}

	m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResRasterizerState::Unload(const std::any& arg)
{
    return S_OK;
}

SPtr<CResRasterizerState> CResRasterizerState::Create()
{
	return ToSPtr(new CResRasterizerState{ "", CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
