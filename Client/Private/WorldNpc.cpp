#include "pch.h"
#include "WorldNpc.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "DbgLineRender.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "UIController.h"
#include "UIManager.h"

//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
NS_USING(Client)

CWorldNpc::CWorldNpc()
{
}


CWorldNpc::~CWorldNpc()
{
}

void CWorldNpc::UpdateGUI()
{
	__super::UpdateGUI();

}

HRESULT CWorldNpc::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CWorldNpc::Initialize(void* pArg)
{
	auto NpcDesc = static_cast<NPC_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 3555;

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComPxRigidBody", &Desc, &m_pComRigidBody)))
		{
			MSG_BOX("Create Failed ComPxRigidBody Npc");
			return E_FAIL;
		}
	}

	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComRigidBody;
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.bIsTrigger = false;
		Desc.tFilter = PX_FILTER_DESC{
			.iLayer = ETOUI(COLLISION_LAYER::ENEMY_HURTBOX),
			.iSimulationMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
			//.iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
		};
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = 1.2f });
		if (!Desc.pResMaterial ||
			FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider,
				"ComPxSphereCollider", &Desc, &m_pComSphereCol)))
		{
			MSG_BOX("Create Failed ComPxSphereCollider Npc");
			return E_FAIL;
		}
		if (!m_pComSphereCol->SetQueryEnabled(false))
			return E_FAIL;
	}

	//피직스
	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		const _float fHorizontalScale =
			std::max(std::abs(NpcDesc->vScale.x), std::abs(NpcDesc->vScale.z));
		const _float fVerticalScale = std::abs(NpcDesc->vScale.y);
		const _float3 vCenterOffset{
			NpcDesc->vCCTCenterOffset.x * NpcDesc->vScale.x,
			NpcDesc->vCCTCenterOffset.y * fVerticalScale,
			NpcDesc->vCCTCenterOffset.z * NpcDesc->vScale.z };
		Desc.fHeight = NpcDesc->fCCTHeight * fVerticalScale;
		Desc.fRadius = NpcDesc->fCCTRadius * fHorizontalScale;
		Desc.fStepOffset = NpcDesc->fCCTStepOffset;
		Desc.vPosition = {
			NpcDesc->vPos.x + vCenterOffset.x,
			NpcDesc->vPos.y + vCenterOffset.y,
			NpcDesc->vPos.z + vCenterOffset.z };
		Desc.tFilter = NpcDesc->tFilter;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pCharacterController)))
		{
			return E_FAIL;
		}
	}
	//캐릭컨트롤러
	{
		CComCharacterMoveIntent::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
			"ComCharacterMoveIntent", &Desc, &m_pMoveIntent)))
		{
			return E_FAIL;
		}
	}
	//캐릭 모터
	{
		CComCharacterMotor::DESC Desc{};
		Desc.pMoveIntent = m_pMoveIntent;
		Desc.pCharacterController = m_pCharacterController;
		Desc.fGravity = -9.81f;
		Desc.vControllerCenterOffset = {
			NpcDesc->vCCTCenterOffset.x * NpcDesc->vScale.x,
			NpcDesc->vCCTCenterOffset.y * std::abs(NpcDesc->vScale.y),
			NpcDesc->vCCTCenterOffset.z * NpcDesc->vScale.z };
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor", &Desc, &m_pCharacterMotor)))
		{
			return E_FAIL;
		}
	}

	CComBeHavior::BEHAVIOR_DESC Desc{};
	Desc.OwnerName = "Com_BT";
	Desc.resBeHaviorMajor = NpcDesc->resBeHaviorMajor;
	Desc.resBeHaviorMinor = NpcDesc->resBeHaviorMinor;
	if (FAILED(AddComponentFromProto("BEHAVIOR", "Prototype_Component_BeHavior", "Com_BT", &Desc, &m_pBeHavior)))
	{
		return E_FAIL;
	};
	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}
	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = NpcDesc->LevelTag;
		Desc.sResTag = NpcDesc->ReSourceTag;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	{
		CComAnimator::DESC DescAnim{};
		DescAnim.sComTag = "ComCModelIntance";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Animator", "ComCModelAnimator", &DescAnim, &m_pModelAnimator)))
		{
			return E_FAIL;
		};
	}

	{
		CComCollider::DESC Desc{};
		Desc.eCollType = CollType::Box;
		Desc.vExtents = { 1.f, 1.f, 1.f };
		if (FAILED(AddComponentFromProto("COLLIDER", "Prototype_Component_Collider", "ComColl", &Desc, &m_pComCollider)))
		{
			return E_FAIL;
		};
	}

	if (FAILED(Ready_Fsm(NpcDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Fsm");
		return E_FAIL;
	}

	Ready_BBKeyValue(NpcDesc);

	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);
	GetTransform().SetPosition(XMLoadFloat3(&NpcDesc->vPos));

	m_pComSphereCol->SetQueryEnabled(true);
	m_pModelAnimator->Play_Anim(0, false);
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	return S_OK;
}
HRESULT CWorldNpc::Ready_Fsm(const _string& LevelTag)
{
	//CMon_State::DESC Desc{};
	//if (FAILED(AddComponentFromProto(LevelTag, "Prototype_Component_Mon_FSM", "Mon_Fsm", &Desc, &m_pFsm))) return E_FAIL;
	//
	//
	//if (false == m_pFsm->Add_State(MON_STATE::SPAWN, CWorldNpc_Spawn::Create(LevelTag))) return E_FAIL;
	//
	//if (false == m_pFsm->Add_State(MON_STATE::COMBAT, CWorldNpc_Combat::Create(LevelTag))) return E_FAIL;
	//
	//if (false == m_pFsm->Add_State(MON_STATE::HIT, CWorldNpc_Hit::Create(LevelTag, this))) return E_FAIL;
	//if (false == m_pFsm->Add_State(MON_STATE::DEAD, CWorldNpc_Dead::Create())) return E_FAIL;
	//
	//if (false == m_pFsm->Initialize_State(MON_STATE::SPAWN)) return E_FAIL;


	return S_OK;
}
void CWorldNpc::Ready_BBKeyValue(NPC_DESC* pDesc)
{
	auto pBB = Get_BlackBoard();

	pBB->Set_Value<_float3>(NPC_KEY::STARTPOS, pDesc->vStartPos);
	pBB->Set_Value<_float3>(NPC_KEY::ENDPOS, pDesc->vEndPos);
}
void CWorldNpc::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame)
	{
		SetPendingDestroy();
		return;
	}
	__super::PriorityUpdate(fTimeDelta);
}

void CWorldNpc::Update(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	__super::Update(fTimeDelta);

}

void CWorldNpc::FixedUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CWorldNpc::LateUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	__super::LateUpdate(fTimeDelta);

}


_bool CWorldNpc::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (eType == PLAYER_SKILL_TYPE::ACIENT_LIGHTNING || eType == PLAYER_SKILL_TYPE::ABRA)
		m_iHp = 0;

	if (eType == PLAYER_SKILL_TYPE::END || eType == PLAYER_SKILL_TYPE::DEFAULT)
		return false;

	Damaged(eType);
	if (eType == PLAYER_SKILL_TYPE::ATTACK)
	{
		const auto hUIController = GET_SINGLE(UIManager)->GetUIController();
		if (hUIController.has_value())
		{
			if (auto* pUIController = CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hUIController))
			{
				pUIController->AddFinisher(2.f);
			}
		}
	}
	m_PendingMonTable.eHitType = eType;

	m_bPending = true;
	return true;

}


void CWorldNpc::Set_Gravity(_bool bGravity)
{
	if (bGravity)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	else
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);
}

E::UPtr<CWorldNpc> CWorldNpc::Create()
{
	auto pInstance = E::ToUPtr(new CWorldNpc{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CWorldNpc");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CWorldNpc::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CWorldNpc{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CWorldNpc");
		return nullptr;
	}

	return pInstance;
}
