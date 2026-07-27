#include "pch.h"
#include "Mon_Weapon.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComBeHavior.h"
#include "ComModelInstance.h"
#include "Trail_CPU.h"
NS_USING(Client)

CMon_Weapon::CMon_Weapon()
	: CGameObject{}
{
}

CMon_Weapon::~CMon_Weapon()
{
}

void CMon_Weapon::UpdateGUI()
{
	CGameObject::UpdateGUI();

}

HRESULT CMon_Weapon::InitializePrototype(void* pArg)
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

HRESULT CMon_Weapon::Initialize(void* pArg)
{
	
	auto pDesc = static_cast<WEAPON_DESC*>(pArg);
	m_iBoneSocketIndex = pDesc->iBoneIndex;
	m_ParentHandle	   = pDesc->ParentHandle;
	
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
		Desc.sResTag   = pDesc->WeaponName;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_StaticModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	XMStoreFloat4x4(&m_ParentMatrix, XMMatrixIdentity());
	//test = CGameInstance::Get().Parse_Command("FireSparkQueue.json");
	return S_OK;
}

void CMon_Weapon::PriorityUpdate(E::_float fTimeDelta)
{
	m_bDissolve = false;
}

void CMon_Weapon::Update(E::_float fTimeDelta)
{
	Weapon_Throw(fTimeDelta);
	Dissolve(fTimeDelta);
}

void CMon_Weapon::LateUpdate(E::_float fTimeDelta)
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
		if (auto pBT = iter->GetComponent<CComBeHavior>("Com_BT"))
		{
			if (!m_bThrow && pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW)))
			{
				m_bThrow = true;
				XMStoreFloat3(&m_vLook, pBT->GetGameObject()->GetTransform().GetState(STATE::LOOK));
				XMStoreFloat4x4(&m_ParentMatrix, (XMLoadFloat4x4(&m_ParentMatrix)));
			}
			else if(m_bThrow && !pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW)))
				m_bThrow = false;
		}
	}
	
	GetTransform().SetParentWorldMatrix(m_ParentMatrix);
	GetTransform().Update();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CMon_Weapon::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
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

	uint32_t	iNumMeshes = pModel->Get_NumMeshes();
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
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, m_fDissolveintensity, m_vDissolve, m_fDissolveintensity, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}


	return S_OK;
}

void CMon_Weapon::Weapon_Throw(_float fTimeDelta)
{
	if (!m_bThrow)
		return;
	m_fAngle = 30.f;
	_vector vTargetLook = XMVector3Normalize(XMLoadFloat3(&m_vLook));
	
	_matrix Rot = XMMatrixRotationQuaternion(XMQuaternionRotationAxis(XMVectorSet(0,0,1, 0), XMConvertToRadians(m_fAngle)));
	_matrix matRot = Rot * XMLoadFloat4x4(&m_ParentMatrix);
	matRot.r[3] += vTargetLook * 15.f * fTimeDelta;
	XMStoreFloat4x4(&m_ParentMatrix, matRot);

}

void CMon_Weapon::Dissolve(_float fTimeDelta)
{
	if (auto iter = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle))
	{
		if (auto pBT = iter->GetComponent<CComBeHavior>("Com_BT"))
		{
			if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::DISSOLVE)))
			{
				m_fTick += fTimeDelta;
				_float t = m_fTick / 3.f;
				m_fDissolveintensity = 1 + (0 -1) * t;
				if (t >= 1.f)
				{
					m_fDissolveintensity = 0.f;
					m_fTick = 0.f;
					m_bDissolve = true;
				}
			}
		}
	}
}

E::UPtr<CMon_Weapon> CMon_Weapon::Create()
{
	auto pInstance = E::ToUPtr(new CMon_Weapon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMon_Weapon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CMon_Weapon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CMon_Weapon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMon_Weapon");
		return nullptr;
	}

	return pInstance;
}
