#include "pch.h"
#include "ResComputeShader.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CResComputeShader::Load(const std::any& arg)
{
	auto desc = std::any_cast<CResShader::DESC>(&arg);

	m_eState = STATE::LOADING;
	if (FAILED(CompileShader(desc)))
	{
		MSG_BOX("COMPUTE SHADER COMPILE FAILED");
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	ComPtr<ID3D11ComputeShader> computeShader{};
	if (FAILED(m_pDevice->CreateComputeShader(m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(), nullptr, &computeShader)))
	{
		MSG_BOX_STR(_wstring{ L"CResComputeShader Create Faield Path:" + StringToWString(m_sPath) }.c_str());
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	m_pComputeShader = std::move(computeShader);
	m_eState = STATE::LOADED;

	m_pBlob.Reset();
	m_pErrorBlob.Reset();
	return S_OK;
}

HRESULT CResComputeShader::Unload(const std::any& arg)
{
	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResComputeShader::CResComputeShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResShader{ sPath, pDevice, pContext }
{
	m_sEntryPoint = "CSMain";
	m_sTarget = "cs_5_0";
}

CResComputeShader::~CResComputeShader()
{
}

SPtr<CResComputeShader> CResComputeShader::Create(const _string& sPath)
{
	return ToSPtr(new CResComputeShader{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext()});
}
