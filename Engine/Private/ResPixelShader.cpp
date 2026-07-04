#include "pch.h"
#include "ResPixelShader.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CResPixelShader::Load(const std::any& arg)
{
	auto desc = std::any_cast<CResShader::DESC>(&arg);



	m_eState = STATE::LOADING;
	if (FAILED(CompileShader(desc)))
	{
		MSG_BOX("PIXEL SHADER COMPILE FAILED");
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	if (FAILED(m_pDevice->CreatePixelShader(m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(), nullptr, &m_pPixelShader)))
	{
		MSG_BOX_STR(_wstring{ L"CResPixelShader Create Faield Path:" + StringToWString(m_sPath) }.c_str());
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	m_eState = STATE::LOADED;

	m_pBlob.Reset();
	m_pErrorBlob.Reset();
	return S_OK;
}

HRESULT CResPixelShader::Unload(const std::any& arg)
{
	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResPixelShader::CResPixelShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResShader{ sPath, pDevice, pContext }
{
	m_sEntryPoint = "PSMain";
	m_sTarget = "ps_5_0";
}

CResPixelShader::~CResPixelShader()
{
}

SPtr<CResPixelShader> CResPixelShader::Create(const _string& sPath)
{
	return ToSPtr(new CResPixelShader{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
