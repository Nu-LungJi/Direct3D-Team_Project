#include "pch.h"
#include "MonEffectBall.h"
#include "Client_Resources.h"
#include "Trail_CPU.h"
#include "PhysXManager.h"
#include "BossTMB.h"
#include "ComModelInstance.h"
#include "Player.h"
#include "ComBeHavior.h"
NS_USING(Client)

CMonEffectBall::CMonEffectBall()
	: CGameObject{}
{
}

CMonEffectBall::~CMonEffectBall()
{
}

void CMonEffectBall::UpdateGUI()
{
	CGameObject::UpdateGUI();

}

HRESULT CMonEffectBall::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CMonEffectBall::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	const auto pDesc = static_cast<const MON_BALL*>(pArg);
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_hParent = pDesc->hOwner;
	m_hTarget = pDesc->hTarget;
	m_iBoneIndex = pDesc->iBoneIndex;
	m_tQueryFilter = pDesc->tQueryFilter;
	m_fDamage = pDesc->fDamage;
	GetTransform().Update();
	m_iEffectID = CGameInstance::Get().PlayEffect("Boss_StarBurst_A", *GetTransform().GetWorldMatrix(), _vector{},
		[h = GetHandle()](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
		{
			if (auto pOBJ = CGameInstance::Get().GetGameObjectByHandleT<CMonEffectBall>(h)) {
				pOBJ->m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
			}
		});

	return S_OK;
}

void CMonEffectBall::PriorityUpdate(E::_float fTimeDelta)
{
}

void CMonEffectBall::FixedUpdate(E::_float fTimeDelta)
{
	Chase(fTimeDelta);
}

void CMonEffectBall::Update(E::_float fTimeDelta)
{

	OverlapTest();
	if (m_bHit || m_iEffectID == INVALID_EFFECT_INSTANCE_ID) {
		SetPendingDestroy();
		return;
	}

}

void CMonEffectBall::LateUpdate(E::_float fTimeDelta)
{
	auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
	auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
	CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 0.f, 0.f, 1.f });
	CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
	CGameInstance::Get().GetDbgLineRender()->AddSphere(5.f, XMLoadFloat4x4(&m_CurWorldmat));
	CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
	CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);
}

HRESULT CMonEffectBall::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CMonEffectBall::OverlapTest()
{
	_float3 vPos{};

	memcpy(&vPos, reinterpret_cast<_float*>(&m_CurWorldmat.m[3]), sizeof _float3);
	PX_OVERLAP_DESC   pxOverLabDesc{};
	PX_OVERLAP_RESULT pxOverLapResult{};

	pxOverLabDesc.tFilter = PX_QUERY_FILTER_DESC{ .iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_HURTBOX) };
	pxOverLabDesc.tGeometry = PX_QUERY_GEOMETRY_DESC{ .eType = PX_QUERY_GEOMETRY_TYPE::SPHERE,.fRadius = 2.f };
	pxOverLabDesc.tPose = PX_QUERY_POSE{ .vPosition = vPos };

	auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

	const auto vPreviousColor = pDbgLineRender->GetColor();
	const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
	pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
	pDbgLineRender->SetDepthTest(true);
	pDbgLineRender->AddSphere(2.f, XMMatrixTranslation(vPos.x, vPos.y, vPos.z));
	pDbgLineRender->SetColor(vPreviousColor);
	pDbgLineRender->SetDepthMode(ePreviousDepthMode);

	if (CGameInstance::Get().GetPhysXManager()->Overlap(pxOverLabDesc, pxOverLapResult))
	{
		if (pxOverLapResult.bHit)
		{
			//m_fDamage
			auto pTarget = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(pxOverLapResult.hGameObject);
			CGameInstance::Get().StopEffect(m_iEffectID);
			_float MonDamange = m_fDamage;
			m_bHit = true;

		}
	}
}

void CMonEffectBall::Chase(_float fTimeDelta)
{
	if (auto iter = CGameInstance::Get().GetGameObjectByHandle(m_hParent))
	{
		if (auto pModel = iter->GetComponent<CComModelInstance>("ComCModelIntance"))
		{

			if (pModel->Get_CombinedBoneMatrices().size() >= m_iBoneIndex)
			{
				_matrix Par = XMLoadFloat4x4(&pModel->Get_CombinedBoneMatrices()[m_iBoneIndex]);
				for (uint32_t i = 0; i < 3; ++i)
				{
					Par.r[i] = XMVector3Normalize(Par.r[i]);
				}
				XMStoreFloat4x4(&m_CurWorldmat, Par * XMLoadFloat4x4(pModel->GetGameObject()->GetTransform().GetWorldMatrix()));
				if (auto pBT = iter->GetComponent<CComBeHavior>("Com_BT"))
				{
					if (!pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW)))
					{
						memcpy(&m_vStartLook, reinterpret_cast<_float*>(m_CurWorldmat.m[2]), sizeof _float3);
						_vector vSrcPos = XMVectorSet(0.f, 0.f, 0.f, 1.f);
						_vector vDestPos = XMLoadFloat3(&iter->GetTransform().GetPosition());
					
						if (auto pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget))
						{
							vSrcPos = XMLoadFloat3(&pTarget->GetTransform().GetPosition());
						
						}
						
						XMStoreFloat3(&m_vEndLook, XMVector3Normalize(vSrcPos - vDestPos));
						XMStoreFloat3(&m_vStartLook, XMVector3Normalize(XMLoadFloat3(&m_vStartLook)));
				
					}
					else if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW)))
					{
						_matrix matWorld = XMLoadFloat4x4(&m_CurWorldmat);

						_float3 vScale = _float3(XMVectorGetX(XMVector3Length(matWorld.r[0])), XMVectorGetX(XMVector3Length(matWorld.r[1])),
							XMVectorGetX(XMVector3Length(matWorld.r[2])));

						_vector vCurrentLook = XMVectorLerp(XMLoadFloat3(&m_vStartLook), XMLoadFloat3(&m_vEndLook), 0.5f);

						_vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f,0.f), vCurrentLook));
						_vector vUp    = XMVector3Normalize(XMVector3Cross(vCurrentLook, vRight));

						matWorld.r[0] = vRight * vScale.x;
						matWorld.r[1] = vUp * vScale.y;
						matWorld.r[2] = vCurrentLook * vScale.z;
						
						matWorld.r[3] = matWorld.r[3] * 10.f *fTimeDelta;
						XMStoreFloat4x4(&m_CurWorldmat, matWorld);
					}
				}
				
				
				if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
					CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, m_CurWorldmat);
			}
		}
	}
}

E::UPtr<CMonEffectBall> CMonEffectBall::Create()
{
	auto pInstance = E::ToUPtr(new CMonEffectBall{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMonEffectBall");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CMonEffectBall::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CMonEffectBall{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonEffectBall");
		return nullptr;
	}

	return pInstance;
}
