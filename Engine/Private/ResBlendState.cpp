#include "pch.h"
#include "ResBlendState.h"
#include "GameInstance.h"

NS_USING(Engine)

CResBlendState::CResBlendState(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResource{sPath}
	, m_pDevice{pDevice}
	, m_pContext{pContext}
{
}

CResBlendState::~CResBlendState()
{
}

HRESULT CResBlendState::Load(const std::any& arg)
{
	auto desc = std::any_cast<D3D11_BLEND_DESC>(&arg);
	if (!desc)
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	m_eState = STATE::LOADING;

	{
		if (FAILED(m_pDevice->CreateBlendState(desc, m_pBlendState.GetAddressOf())))
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResBlendState::Unload(const std::any& arg)
{
	return S_OK;
}

SPtr<CResBlendState> CResBlendState::Create()
{
	return ToSPtr(new CResBlendState{ "", CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
