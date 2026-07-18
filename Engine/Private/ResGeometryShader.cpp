#include "pch.h"
#include "ResGeometryShader.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CResGeometryShader::Load(const std::any& arg)
{
	m_eState = STATE::LOADING;
	if (FAILED(CompileShader()))
	{
		MSG_BOX("GEOMETRY SHADER COMPILE FAILED");
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	ComPtr<ID3D11GeometryShader> geometryShader{};
	if (FAILED(m_pDevice->CreateGeometryShader(m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(), nullptr, &geometryShader)))
	{
		MSG_BOX_STR(_wstring{ L"CResGeometryShader Create Faield Path:" + StringToWString(m_sPath) }.c_str());
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}
	m_pGeometryShader = std::move(geometryShader);
	m_eState = STATE::LOADED;

	m_pBlob.Reset();
	m_pErrorBlob.Reset();
	return S_OK;
}

HRESULT CResGeometryShader::Unload(const std::any& arg)
{
	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResGeometryShader::CResGeometryShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResShader{ sPath, pDevice, pContext }
{
	m_sEntryPoint = "GSMain";
	m_sTarget = "gs_5_0";
}

CResGeometryShader::~CResGeometryShader()
{
}

SPtr<CResGeometryShader> CResGeometryShader::Create(const _string& sPath)
{
	return ToSPtr(new CResGeometryShader{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
