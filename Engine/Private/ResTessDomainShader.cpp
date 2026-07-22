#include "pch.h"
#include "ResTessDomainShader.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CResTessDomainShader::Load(const std::any& arg)
{
	m_eState = STATE::LOADING;
	if (FAILED(CompileShader()))
	{
		MSG_BOX("CResTessDomainShader SHADER COMPILE FAILED");
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	ComPtr<ID3D11DomainShader> domainShader{};
	if (FAILED(m_pDevice->CreateDomainShader(m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(), nullptr, &domainShader)))
	{
		MSG_BOX_STR(_wstring{ L"CResTessDomainShader Create Faield Path:" + StringToWString(m_sPath) }.c_str());
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	m_pTessDomainShader = std::move(domainShader);
	m_eState = STATE::LOADED;

	m_pBlob.Reset();
	m_pErrorBlob.Reset();
	return S_OK;
}

HRESULT CResTessDomainShader::Unload(const std::any& arg)
{
	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResTessDomainShader::CResTessDomainShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResShader{ sPath, pDevice, pContext }
{
	m_sEntryPoint = "DSMain";
	m_sTarget = "ds_5_0";
}

CResTessDomainShader::~CResTessDomainShader()
{
}

SPtr<CResTessDomainShader> CResTessDomainShader::Create(const _string& sPath)
{
	return ToSPtr(new CResTessDomainShader{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
