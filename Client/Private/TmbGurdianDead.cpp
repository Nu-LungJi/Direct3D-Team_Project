#include "pch.h"
#include "TmbGurdianDead.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComModelInstance.h"
#include "Trail_CPU.h"
NS_USING(Client)

CTmbGurdianDead::CTmbGurdianDead()
	: CGameObject{}
{
}

CTmbGurdianDead::~CTmbGurdianDead()
{
}

void CTmbGurdianDead::UpdateGUI()
{
	CGameObject::UpdateGUI();

}

HRESULT CTmbGurdianDead::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (FAILED(m_pResPixelNonAnimShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CTmbGurdianDead::Initialize(void* pArg)
{

	auto pDesc = static_cast<WEAPON_DESC*>(pArg);
	m_iBoneSocketIndex = pDesc->iBoneIndex;
	m_ParentHandle = pDesc->ParentHandle;

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

	{
		CComStaticModelInstance::DESC Desc{};
		Desc.sGroupTag = pDesc->LevelTag;
		Desc.sResTag = pDesc->WeaponName;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_StaticModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	XMStoreFloat4x4(&m_ParentMatrix, XMMatrixIdentity());
	GetTransform().SetScale(_float3{ 4.f,4.f,4.f });

	//test = CGameInstance::Get().Parse_Command("FireSparkQueue.json");
	return S_OK;
}

void CTmbGurdianDead::PriorityUpdate(E::_float fTimeDelta)
{
}

void CTmbGurdianDead::Update(E::_float fTimeDelta)
{
}

void CTmbGurdianDead::LateUpdate(E::_float fTimeDelta)
{
	if (auto iter = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle))
	{
		if (!m_bThrow)
		{
			if (auto pModel = iter->GetComponent<CComModelInstance>("ComCModelIntance"))
			{
				if (pModel->Get_CombinedBoneMatrices().size() >= m_iBoneSocketIndex)
				{
					_matrix Par = XMLoadFloat4x4(&pModel->Get_CombinedBoneMatrices()[m_iBoneSocketIndex]);
					for (uint32_t i = 0; i < 3; ++i)
					{
						Par.r[i] = XMVector3Normalize(Par.r[i]);
					}
					XMStoreFloat4x4(&m_ParentMatrix, Par * XMLoadFloat4x4(pModel->GetGameObject()->GetTransform().GetWorldMatrix()));
				}
			}
		}

	}

	GetTransform().SetParentWorldMatrix(m_ParentMatrix);
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CTmbGurdianDead::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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

	const auto& vs = m_pResVertexNonAnimShader;
	const auto& ps = m_pResPixelNonAnimShader;

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = m_pComModelInstance->GetModel();

	uint32_t   iNumMeshes = pModel->Get_NumMeshes();
	for (uint32_t i = 0; i < iNumMeshes; ++i) {
		const auto& viBuffer = pModel->GetMeshes()[i];


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

		{
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);   // EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}


	return S_OK;
}


E::UPtr<CTmbGurdianDead> CTmbGurdianDead::Create()
{
	auto pInstance = E::ToUPtr(new CTmbGurdianDead{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTmbGurdianDead");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTmbGurdianDead::Clone(void* pArg)
{
	auto   pInstance = E::ToUPtr(new CTmbGurdianDead{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTmbGurdianDead");
		return nullptr;
	}

	return pInstance;
}
