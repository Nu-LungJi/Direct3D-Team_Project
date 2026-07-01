#include "pch.h"
#include "ResVertexShader.h"
#include "GameInstance.h"

NS_USING(Engine)

HRESULT CResVertexShader::Load(const std::any& arg)
{
	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	m_eState = STATE::LOADING;

	if (FAILED(CompileShader()))
	{
		MSG_BOX("VERTEX SHADER COMPILE FAILED");
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	if (FAILED(m_pDevice->CreateVertexShader(m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(), nullptr, &m_pVertexShader)))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	ComPtr<ID3D11ShaderReflection> pReflector{};
	if (FAILED(D3DReflect(
		m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(),
		IID_ID3D11ShaderReflection,
		(void**)&pReflector
	)))
	{
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	D3D11_SHADER_DESC shaderDesc;
	pReflector->GetDesc(&shaderDesc);

	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout;

    for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
    {
        D3D11_SIGNATURE_PARAMETER_DESC paramDesc{};
        pReflector->GetInputParameterDesc(i, &paramDesc);

        D3D11_INPUT_ELEMENT_DESC element{};
        element.SemanticName = paramDesc.SemanticName;
        element.SemanticIndex = paramDesc.SemanticIndex;
        element.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;

        if (strncmp(paramDesc.SemanticName, "INSTANCE_", 9) == 0)
        {
            element.InputSlot = 1;
            element.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
            element.InstanceDataStepRate = 1;
        }
        else
        {
            element.InputSlot = 0;
            element.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            element.InstanceDataStepRate = 0;
        }

        if (strcmp(paramDesc.SemanticName, "COLOR_PACK") == 0)
        {
            element.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        else if (paramDesc.Mask == 1)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                element.Format = DXGI_FORMAT_R32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                element.Format = DXGI_FORMAT_R32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                element.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (paramDesc.Mask <= 3)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                element.Format = DXGI_FORMAT_R32G32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                element.Format = DXGI_FORMAT_R32G32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                element.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (paramDesc.Mask <= 7)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                element.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                element.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                element.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (paramDesc.Mask <= 15)
        {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                element.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                element.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                element.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        if (element.Format == DXGI_FORMAT_UNKNOWN)
        {
            return E_FAIL;
        }

        inputLayout.push_back(element);
    }
	m_inputLayout = inputLayout;

	{
		auto hr = m_pDevice->CreateInputLayout(
			inputLayout.data(),
			(UINT)inputLayout.size(),
			m_pBlob->GetBufferPointer(),
			m_pBlob->GetBufferSize(),
			&m_pInputLayout
		);

		if (FAILED(hr))
		{
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}

	}

	m_eState = STATE::LOADED;

	m_pBlob.Reset();
	m_pErrorBlob.Reset();
	return S_OK;
}

HRESULT CResVertexShader::Unload(const std::any& arg)
{
	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResVertexShader::CResVertexShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResShader{ sPath, pDevice , pContext }
{
	m_sEntryPoint = "VSMain";
	m_sTarget = "vs_5_0";
}

CResVertexShader::~CResVertexShader()
{
}

SPtr<CResVertexShader> CResVertexShader::Create(const _string& sPath)
{
	return ToSPtr(new CResVertexShader{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
