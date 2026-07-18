#include "pch.h"
#include "ResTessHullShader.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CResTessHullShader::Load(const std::any& arg)
{
	m_eState = STATE::LOADING;
	if (FAILED(CompileShader()))
	{
		MSG_BOX("CResTessHullShader SHADER COMPILE FAILED");
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	ComPtr<ID3D11HullShader> hullShader{};
	if (FAILED(m_pDevice->CreateHullShader(m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(), nullptr, &hullShader)))
	{
		MSG_BOX_STR(_wstring{ L"CResTessHullShader Create Faield Path:" + StringToWString(m_sPath) }.c_str());
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	m_pTessHullShader = std::move(hullShader);
	m_eState = STATE::LOADED;

	m_pBlob.Reset();
	m_pErrorBlob.Reset();
	return S_OK;
}

HRESULT CResTessHullShader::Unload(const std::any& arg)
{
	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResTessHullShader::CResTessHullShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResShader{ sPath, pDevice, pContext }
{
	m_sEntryPoint = "HSMain";
	m_sTarget = "hs_5_0";
}

CResTessHullShader::~CResTessHullShader()
{
}

SPtr<CResTessHullShader> CResTessHullShader::Create(const _string& sPath)
{
	return ToSPtr(new CResTessHullShader{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
