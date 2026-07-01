#include "pch.h"
#include "TestModel.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "GameInstance.h"

NS_USING(Client)

CTestModel::CTestModel()
	: CGameObject{}
{
}

CTestModel::~CTestModel()
{
}

HRESULT CTestModel::InitializePrototype(void* pArg)
{
	m_pResTestModel = CGameInstance::Get().GetResourceFirst<CResTestModel>("TEST", "Model_Resource");
	if (!m_pResTestModel)
	{
		return E_FAIL;
	}



	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	//m_pResVertexShader = CResVertexShader::Create("./Resources/SampleClient/Shader/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	//m_pResPixelShader = CResPixelShader::Create("./Resources/SampleClient/Shader/Shader_VtxNorTex.hlsl");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	m_pResSamplerState = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (!m_pResSamplerState)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CTestModel::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}


	return S_OK;
}

void CTestModel::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTestModel::Update(E::_float fTimeDelta)
{
	m_pResTestModel->Play_Animation(fTimeDelta);
}

void CTestModel::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CTestModel::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}
	const auto& vs = m_pResVertexShader;
	const auto& ps = m_pResPixelShader;
	
	

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);


	uint32_t	iNumMeshes = m_pResTestModel->Get_NumMeshes();


	for (uint32_t i = 0; i < iNumMeshes; ++i) {
		const auto& viBuffer = m_pResTestModel->GetMeshes()[i];


		ID3D11Buffer* vertexBuffers[] = {
				viBuffer->GetVertexBuffer().Get()
		};
		uint32_t strides[] = {
			viBuffer->GetVertexStride()
		};
		uint32_t offsets[] = {
			0
		};
		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		//{
		//	auto tex = m_pResTestModel->GetMaterials()[0]->GetTextures()[1][0];
		//	pContext->PSSetShaderResources(0, 1, tex->GetSRV().GetAddressOf());
		//}

		{
			m_pResTestModel->Bind_Materials(i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0);
		
		}

		{
			m_pResTestModel->Bind_BoneMatrices(i);
		}

		{
			const auto& sampler = m_pResSamplerState;
			pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
		}

		{
			const auto& rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
			pContext->RSSetState(rasterizer->GetRasterizerState().Get());
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}


	return S_OK;
}

E::UPtr<CTestModel> CTestModel::Create()
{
	auto pInstance = E::ToUPtr(new CTestModel{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestModel");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestModel::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestModel{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestModel");
		return nullptr;
	}

	return pInstance;
}
