#include "pch.h"

#include "ResFontCustom.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CResFontCustom::Load(const std::any& arg)
{
	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}
	m_eState = STATE::LOADING;
	{
		m_pFont = std::make_unique<SpriteFont>(m_pDevice.Get(), StringToWString(m_sPath).c_str());
		if (!m_pFont)
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}
	}
	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResFontCustom::Unload(const std::any& arg)
{
	return S_OK;
}

CResFontCustom::CResFontCustom(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResFont{sPath, pDevice ,pContext }
{
}

CResFontCustom::~CResFontCustom()
{
}

SPtr<CResFontCustom> CResFontCustom::Create(const _string& sPath)
{
	return ToSPtr(new CResFontCustom{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
