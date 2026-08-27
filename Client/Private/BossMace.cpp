#include "pch.h"
#include "BossMace.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComBeHavior.h"
#include "ComModelInstance.h"
#include "Trail_CPU.h"
#include "BossTMB.h"

NS_USING(Client)

CBossMace::CBossMace()
	: CMon_Weapon{}
{
}

CBossMace::~CBossMace()
{
}

void CBossMace::UpdateGUI()
{
	CMon_Weapon::UpdateGUI();

	ImGui::DragFloat("eqwewq", &m_fTime, 0.1f, 0.f, 10.f);
}

HRESULT CBossMace::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
		return E_FAIL;

	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, TAG_RES_PERMANENT_NONBLENDSHADER);
	if (FAILED(m_pResPixelNonAnimShader->Load()))
		return E_FAIL;

	return S_OK;
}

HRESULT CBossMace::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_vEmissive = _float3(0.213f, 0.243f, 0.5f);
	m_fMaxEmissiveIntensity = 50.f;
	m_fTime = 15.f;
	return S_OK;
}

void CBossMace::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_bDead)
		return;
	__super::PriorityUpdate(fTimeDelta);
}

void CBossMace::Update(E::_float fTimeDelta)
{
	if (m_bDead)
	{
		SetPendingDestroy();
		return;
	}
	__super::Update(fTimeDelta);
	Enable_Emissive(fTimeDelta);
	Disable_Emissive(fTimeDelta);
}

void CBossMace::LateUpdate(E::_float fTimeDelta)
{
	if (m_bDead)
		return;
	if (auto iter = CGameInstance::Get().GetGameObjectByHandleT<CBossTMB>(m_ParentHandle))
	{
		if (auto pBT = iter->GetComponent<CComBeHavior>("Com_BT"))
		{	
			if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::DEAD)))
			{
				m_bDead = true;
				return;
			}
				
			if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::GROGY)))
			{
				m_bEmissive = false;
				m_fEmissiveIntensity = 0.f;
				return;
			}
			if (!pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW))|| iter->Get_CurSkillName() != "MorningStarAfterEffect")
			{
				m_bEmissive = false;
				return;
			}

			m_bEmissive = true;
			if (auto pModel = iter->GetComponent<CComModelInstance>("ComCModelIntance"))
			{
				if (pModel->Get_CombinedBoneMatrices().size() > m_iBoneSocketIndex)
				{
					_matrix Par = XMLoadFloat4x4(&pModel->Get_CombinedBoneMatrices()[m_iBoneSocketIndex]);
					for (uint32_t i = 0; i < 3; ++i)
					{
						Par.r[i] = XMVector3Normalize(Par.r[i]);
					}
					XMStoreFloat4x4(&m_ParentMatrix, Par * XMLoadFloat4x4(pModel->GetGameObject()->GetTransform().GetWorldMatrix()));

					GetTransform().SetParentWorldMatrix(m_ParentMatrix);
					GetTransform().Update();

					CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
					CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
				}
			}
		}
	}
}


HRESULT CBossMace::Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject)
		return E_FAIL;
	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return E_FAIL;

		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	const auto model = m_pComModelInstance->GetModel();
	if (!model)	return E_FAIL;

	for (uint32_t i = 0; i < model->Get_NumMeshes(); ++i)
	{
		const auto& viBuffer = model->GetMeshes()[i];
		ID3D11Buffer* vertexBuffer = viBuffer->GetVertexBuffer().Get();
		const uint32_t stride = viBuffer->GetVertexStride();
		const uint32_t offset = 0;

		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());
		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	return S_OK;
}
void CBossMace::Active_Effect(const _string& EffectName)
{
	if (!m_bActive)
	{
		const _matrix effectWorld =
			XMMatrixTranslation(
				m_ParentMatrix._41,
				m_ParentMatrix._42,
				m_ParentMatrix._43);	
		_float4x4 effectMat;
		XMStoreFloat4x4(&effectMat, effectWorld);
		m_iEffectID = CGameInstance::Get().PlayEffect(EffectName, effectMat, _vector{},
			[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
			{
				if (effectId != m_iEffectID)
					return;
				m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
			});
		m_bActive = true;
	}
}
void CBossMace::Reset_Active()
{
	if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
	{
		const EFFECT_INSTANCE_ID iOldEffect = m_iEffectID;
		m_iEffectID = INVALID_EFFECT_INSTANCE_ID;

		CGameInstance::Get().StopEffect(iOldEffect);
	}
	m_bActive = false;

}
void CBossMace::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
}

void CBossMace::Enable_Emissive(_float fTimeDelta)
{	
	if (!m_bEmissive) return;

	if (m_fEmissiveIntensity >= m_fMaxEmissiveIntensity)
	{
		m_fTick = 0.f;
		return;
	}
	m_fTick += fTimeDelta;

	_float t = m_fTick / m_fTime;
	
	if (t >= 1.f)
	{
		t = 1.f;
	}
	
	m_fEmissiveIntensity = 0.f + (m_fMaxEmissiveIntensity - 0.f) * t;
}
void CBossMace::Disable_Emissive(_float fTimeDelta)
{
	if (m_bEmissive) return;
	
		m_fEmissiveIntensity = m_fTick = 0.f;
}
E::UPtr<CBossMace> CBossMace::Create()
{
	auto pInstance = E::ToUPtr(new CBossMace{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBossMace");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CBossMace::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBossMace{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBossMace");
		return nullptr;
	}

	return pInstance;
}
