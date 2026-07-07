#include "pch.h"
#include "MapMeshObject.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

CMapMeshObject::CMapMeshObject()
	: CGameObject{}
{
}

CMapMeshObject::CMapMeshObject(const CMapMeshObject& Prototype)
	: CGameObject{ Prototype }
	, m_pResVertexShader{ Prototype.m_pResVertexShader }
	, m_pResPixelShader{ Prototype.m_pResPixelShader }
	, m_pResSamplerState{ Prototype.m_pResSamplerState }
{
}

CMapMeshObject::~CMapMeshObject()
{
}

HRESULT CMapMeshObject::InitializePrototype(void* pArg)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}

	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))
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

HRESULT CMapMeshObject::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	auto* pDesc = static_cast<MAP_MESH_OBJECT_DESC*>(pArg);
	if (pDesc == nullptr)
	{
		return E_FAIL;
	}

	m_modelResourceGroup = pDesc->modelGroupTag;
	m_modelResourceTag = pDesc->modelResTag;

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = m_modelResourceGroup;
		Desc.sResTag = m_modelResourceTag;
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelInstance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	return S_OK;
}

void CMapMeshObject::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CMapMeshObject::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	if (m_pComModelInstance == nullptr || m_pComModelInstance->GetModel() == nullptr)
	{
		return E_FAIL;
	}

	{
		CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	auto pModel = m_pComModelInstance->GetModel();
	const uint32_t iNumMeshes = pModel->Get_NumMeshes();
	for (uint32_t i = 0; i < iNumMeshes; ++i)
	{
		const auto& viBuffer = pModel->GetMeshes()[i];

		ID3D11Buffer* vertexBuffers[] = { viBuffer->GetVertexBuffer().Get() };
		uint32_t strides[] = { viBuffer->GetVertexStride() };
		uint32_t offsets[] = { 0 };
		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		m_pComModelInstance->Bind_Materials(pContext, i, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0);
		pContext->PSSetSamplers(0, 1, m_pResSamplerState->GetSamplerState().GetAddressOf());

		const auto& rasterizer = CGameInstance::GetConst().GetResourceFirst<CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
		if (rasterizer)
		{
			pContext->RSSetState(rasterizer->GetRasterizerState().Get());
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

void CMapMeshObject::UpdateGUI()
{
	CGameObject::UpdateGUI();
}

HRESULT CMapMeshObject::SetModelResource(const std::string& modelGroupTag, const std::string& modelResTag)
{
	if (m_pComModelInstance == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(m_pComModelInstance->ChangeModel(modelGroupTag, modelResTag)))
	{
		return E_FAIL;
	}

	m_modelResourceGroup = modelGroupTag;
	m_modelResourceTag = modelResTag;

	return S_OK;
}

UPtr<CMapMeshObject> CMapMeshObject::Create()
{
	auto pInstance = ToUPtr(new CMapMeshObject{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMapMeshObject");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CMapMeshObject::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CMapMeshObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMapMeshObject");
		return nullptr;
	}

	return pInstance;
}
