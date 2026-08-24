#include "pch.h"
#include "Troll.h"
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
//FSM
#include "Mon_State.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
#include "Troll_Hit.h"
#include "Troll_Combat.h"
#include "Troll_Spawn.h"
NS_USING(Client)

CTroll::CTroll()
{
}


CTroll::~CTroll()
{
}

void CTroll::UpdateGUI()
{
	__super::UpdateGUI();
	ImGui::DragInt("HP", &m_iHp, 0, 1);

}

HRESULT CTroll::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CTroll::Initialize(void* pArg)
{
	auto MonDesc = static_cast<TROLL_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 555;

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComPxRigidBody", &Desc, &m_pComRigidBody)))
		{
			MSG_BOX("Create Failed ComPxRigidBody EnderDragon");
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
			MSG_BOX("Create Failed ComPxSphereCollider EnderDragon");
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
			std::max(std::abs(MonDesc->vScale.x), std::abs(MonDesc->vScale.z));
		const _float fVerticalScale = std::abs(MonDesc->vScale.y);
		const _float3 vCenterOffset{
			MonDesc->vCCTCenterOffset.x * MonDesc->vScale.x,
			MonDesc->vCCTCenterOffset.y * fVerticalScale,
			MonDesc->vCCTCenterOffset.z * MonDesc->vScale.z };
		Desc.fHeight = MonDesc->fCCTHeight * fVerticalScale;
		Desc.fRadius = MonDesc->fCCTRadius * fHorizontalScale;
		Desc.fStepOffset = MonDesc->fCCTStepOffset;
		Desc.vPosition = {
			MonDesc->vPos.x + vCenterOffset.x,
			MonDesc->vPos.y + vCenterOffset.y,
			MonDesc->vPos.z + vCenterOffset.z };
		Desc.tFilter = MonDesc->tFilter;
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
			MonDesc->vCCTCenterOffset.x * MonDesc->vScale.x,
			MonDesc->vCCTCenterOffset.y * std::abs(MonDesc->vScale.y),
			MonDesc->vCCTCenterOffset.z * MonDesc->vScale.z };
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
	Desc.resBeHaviorMajor = MonDesc->resBeHaviorMajor;
	Desc.resBeHaviorMinor = MonDesc->resBeHaviorMinor;
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
		Desc.sGroupTag = MonDesc->LevelTag;
		Desc.sResTag = MonDesc->ReSourceTag;

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

	if (FAILED(Ready_Fsm(MonDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Fsm");
		return E_FAIL;
	}
	if (FAILED(Ready_Skill(MonDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Skill");
		return E_FAIL;
	}

	Ready_BBKeyValue();

	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);

	GetTransform().SetPosition(XMLoadFloat3(&MonDesc->vPos));
	m_eMonType = MONSTER_TYPE::BOSS;

	ReadySound();
	m_pComSphereCol->SetQueryEnabled(true);
	m_iColliderBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("chest_targetSocket");
	return S_OK;
}
void CTroll::ReadySound()
{
}
HRESULT CTroll::Ready_Fsm(const _string& LevelTag)
{
	CMon_State::DESC Desc{};
	if (FAILED(AddComponentFromProto(LevelTag, "Prototype_Component_Mon_FSM", "Mon_Fsm", &Desc, &m_pFsm))) return E_FAIL;
	
	if (false == m_pFsm->Add_State(MON_STATE::SPAWN, CTroll_Spawn::Create(LevelTag))) return E_FAIL;
	
	if (false == m_pFsm->Add_State(MON_STATE::COMBAT, CTroll_Combat::Create())) return E_FAIL;
	
	if (false == m_pFsm->Add_State(MON_STATE::HIT, CTroll_Hit::Create())) return E_FAIL;

	//if (false == m_pFsm->Add_State(MON_STATE::HIT, CMon_Godae::Create())) return E_FAIL;

	if (false == m_pFsm->Initialize_State(MON_STATE::SPAWN)) return E_FAIL;


	return S_OK;
}
HRESULT CTroll::Ready_Skill(const _string& LevelTag)
{
	if (ETOUI(TROLL_SKILL::END) > ETOUI(ATTMON::END))
		return E_FAIL;

	m_MonSkillLists[ATTMON::SLOT0] = ETOUI(TROLL_SKILL::BOOM);
	//////////////////////파티클 넣는곳/////////////////////////
	m_EffectNames[ETOUI(TROLL_SKILL::BOOM)] = "FireBall";
	////////////////////////////////////////////////////////////

	return S_OK;
}
void CTroll::Ready_BBKeyValue()
{

}
void CTroll::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame)
	{
		SetPendingDestroy();
		return;
	}
	if (CGameInstance::Get().KeyPressing(DIK_LSHIFT) && CGameInstance::Get().KeyDown(DIK_H))
		m_bDebug = !m_bDebug;

	if (!m_bDebug) return;

	m_pFsm->PriorityUpdate(fTimeDelta);
	Update_BBToFsm();
	__super::PriorityUpdate(fTimeDelta);
	m_pFsm->Update(fTimeDelta);
}

void CTroll::Update(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	if (!m_bDebug) return;
	__super::Update(fTimeDelta);
}

void CTroll::Stuck()
{
	if (nullptr == m_pFsm) return;
	m_pFsm->Request_State(MON_STATE::GODAE);
}

void CTroll::FixedUpdate(E::_float fTimeDelta)
{
	if (m_bEndGame) return;
	if (!m_bDebug) return;
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CTroll::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bDebug) return;
	m_pFsm->LateUpdate(fTimeDelta);
	__super::LateUpdate(fTimeDelta);

}

void CTroll::Set_StateFinished(_bool bFinished)
{
	//스테이트가 완료된 판정에 대해서 다시 초기회
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;

	pBB->Set_Value(EDG_KEY::BSTATE_FINISHED, bFinished);
}
_bool CTroll::Is_StateFinished()
{
	//스테이트가 끝났는지 확인
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return false;

	auto pbFinished = pBB->Get_Value<_bool>(EDG_KEY::BSTATE_FINISHED);
	if (nullptr == pbFinished) return false;

	return *pbFinished;
}
_string CTroll::Get_SkillName(ATTMON SkillNode)
{
	auto pValue = m_MonSkillLists.find(SkillNode);

	if (pValue == m_MonSkillLists.end())
		return "";

	if (pValue->second >= ETOUI(TROLL_SKILL::END))
		return "";

	return MagicEnumToStringView(static_cast<TROLL_SKILL>(pValue->second)).data();
}

_bool CTroll::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (m_iHp <= 0 && m_pFsm->GetCurState() == MON_STATE::DEAD)
		return false;

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

	if (true == BreakSkillType(eType) && false == m_bIsBreak)
	{
		m_pFsm->Request_State(MON_STATE::HIT);
		m_bIsBreak = true;

		m_PendingMonTable.eAttType = m_eAttType;
		m_PendingMonTable.eHitType = PLAYER_SKILL_TYPE::DESTORY;

		m_bPending = true;
	}

	return true;

}

void CTroll::Set_AttTable(ATTMON eType, _float2 fSkillRatio)
{
	if (eType == ATTMON::END)
		return;

	auto pbEffect = Get_BlackBoard()->Get_Value<_bool>(EDG_KEY::EDGEFFECT);
	uint32_t iSkillNum = Find_SkillNum(eType);
	if (iSkillNum == UINT_MAX || iSkillNum >= ETOUI(TROLL_SKILL::END))
		return;

	if (*pbEffect)
	{
		_float4x4 mat;
		XMStoreFloat4x4(&mat, GetTransform().GetLoadedWorldMatrix());
		CGameInstance::Get().Spawn(m_EffectNames[iSkillNum], mat);
		Get_BlackBoard()->Set_Value<_bool>(EDG_KEY::EDGEFFECT, false);
	}

	m_CurEffectName.clear();
	m_eAttType = ATTMON::END;
	m_eLastSkillTable = m_eAttType = eType;

}
void CTroll::Flag_Check(_float fTimeDelta)
{
	
}
void CTroll::Update_BBToFsm()
{
	auto pBB = Get_BlackBoard();

	if (nullptr == pBB)
		return;

	pBB->Set_Value(EDG_KEY::STATE, m_pFsm->GetCurState());
}
_bool CTroll::BreakSkillType(PLAYER_SKILL_TYPE eType)
{
	uint32_t iSkillNumber = Find_SkillNum(m_eAttType);
	//파훼 됨?
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ACCIO:
		//if (ETOUI(DRAGON_SKILL::FIREBALL) == iSkillNumber)
		//	return true;
		break;
	case PLAYER_SKILL_TYPE::DEPULSO:
		break;

	case PLAYER_SKILL_TYPE::DESCENDO:
		break;

	case PLAYER_SKILL_TYPE::ACIENT_LIGHTNING:
		return true;
		break;
	case PLAYER_SKILL_TYPE::DESTORY:
		return true;
		break;
	case PLAYER_SKILL_TYPE::ABRA:
		return true;
		break;
	}
	return false;
}

E::UPtr<CTroll> CTroll::Create()
{
	auto pInstance = E::ToUPtr(new CTroll{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTroll");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTroll::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTroll{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTroll");
		return nullptr;
	}

	return pInstance;
}
