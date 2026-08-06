#include "pch.h"
#include "EnderDragon.h"
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
#include "TmbGurdianDead.h"
#include "GurdianWeapon.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "UIController.h"
#include "UIManager.h"
#include "EnderDragon_State.h"
//FSM
#include "Edg_Spawn.h"
#include "Edg_Combat.h"
#include "Edg_Hit.h"
#include "Edg_Phase.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
NS_USING(Client)

CEnderDragon::CEnderDragon()
{
}


CEnderDragon::~CEnderDragon()
{
}

void CEnderDragon::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::DragInt("HP", &m_iHp, 0, 1);
}

HRESULT CEnderDragon::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	return S_OK;
}

HRESULT CEnderDragon::Initialize(void* pArg)
{
	auto MonDesc = static_cast<DRAGON_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 555800;
	
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
	if (FAILED(Ready_Skill()))
	{
		MSG_BOX("Create Faield Skill");
		return E_FAIL;
	}

	//m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	auto pBB = Get_BlackBoard();
	pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, m_ePhase);

	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);
	GetTransform().SetPosition(XMLoadFloat3(&MonDesc->vPos));
	return S_OK;
}

void CEnderDragon::PriorityUpdate(E::_float fTimeDelta)
{
	Check_Phase();
	Flag_Check(fTimeDelta);
	m_pFsm->PriorityUpdate(fTimeDelta);
	Update_BBToFsm();
	__super::PriorityUpdate(fTimeDelta);
}

void CEnderDragon::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	m_pFsm->Update(fTimeDelta);
}

void CEnderDragon::FixedUpdate(E::_float fTimeDelta)
{
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CEnderDragon::LateUpdate(E::_float fTimeDelta)
{
	m_pFsm->LateUpdate(fTimeDelta);
	__super::LateUpdate(fTimeDelta);

}
HRESULT CEnderDragon::Ready_Fsm(const _string& LevelTag)
{
	CEnderDragon_State::DESC Desc{};
	if (FAILED(AddComponentFromProto(LevelTag,"Prototype_Component_Dragon_FSM","EnderDragon_Fsm",&Desc,&m_pFsm))) return E_FAIL;
	
	
	if (false == m_pFsm->Add_State(EDG_STATE::SPAWN, CEdg_Spawn::Create())) return E_FAIL;

	if (false == m_pFsm->Add_State(EDG_STATE::COMBAT, CEdg_Combat::Create())) return E_FAIL;

	if (false == m_pFsm->Add_State(EDG_STATE::HIT, CEdg_Hit::Create())) return E_FAIL;

	if (false == m_pFsm->Add_State(EDG_STATE::PHASE_CHANGE, CEdg_Phase::Create())) return E_FAIL;

	if (false == m_pFsm->Initialize_State(EDG_STATE::SPAWN)) return E_FAIL;


	return S_OK;
}

HRESULT CEnderDragon::Ready_Skill()
{
	if (ETOUI(DRAGON_SKILL::END) > ETOUI(ATTMON::END))
		return E_FAIL;

	m_MonSkillLists[ATTMON::SLOT0] = ETOUI(DRAGON_SKILL::BOOM);
	m_MonSkillLists[ATTMON::SLOT1] = ETOUI(DRAGON_SKILL::BRESS);
	m_MonSkillLists[ATTMON::SLOT2] = ETOUI(DRAGON_SKILL::FIREBALL);
	
	m_MonSkillLists[ATTMON::SKIP] = ETOUI(DRAGON_SKILL::SKIP);


	//m_EffectNames[ETOUI(DRAGON_SKILL::JUMP_START)] = "TombJumpStart";
	//m_EffectNames[ETOUI(DRAGON_SKILL::JUMP_END)] = "TombJumpEnd";
	//m_EffectNames[ETOUI(DRAGON_SKILL::HIT_ACCIO)] = "AccioGrab";
	
	
	return S_OK;
}

void CEnderDragon::Set_StateFinished(_bool bFinished)
{
	//스테이트가 완료된 판정에 대해서 다시 초기회
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;

	pBB->Set_Value(EDG_KEY::BSTATE_FINISHED, bFinished);
}
_bool CEnderDragon::Is_StateFinished()
{
	//스테이트가 끝났는지 확인
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return false;

	auto pbFinished = pBB->Get_Value<_bool>(EDG_KEY::BSTATE_FINISHED);
	if (nullptr == pbFinished) return false;
	
	return *pbFinished;
}
_string CEnderDragon::Get_SkillName(ATTMON SkillNode)
{
	auto pValue = m_MonSkillLists.find(SkillNode);

	if (pValue == m_MonSkillLists.end())
		return "";

	if (pValue->second >= ETOUI(DRAGON_SKILL::END))
		return "";

	return MagicEnumToStringView(static_cast<DRAGON_SKILL>(pValue->second)).data();
}
CBTBlackBoard* CEnderDragon::Get_BlackBoard()
{
	if (nullptr == m_pBeHavior) return nullptr;
	return m_pBeHavior->Get_Blackboard();
}
_bool CEnderDragon::Check_Table(PLAYER_SKILL_TYPE eType)
{
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
		m_pFsm->Request_State(EDG_STATE::HIT);
		m_bIsBreak = true;
		MON_HIT_INFO HitInfo{};
		HitInfo.eAttType = m_eAttType;
		HitInfo.eHitType = eType;
		m_PendingMonTable = HitInfo;
		m_bPending = true;
	}
	
	return true;

}
void CEnderDragon::Check_Phase()
{
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;

	if (false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE2)] && m_iHp <= m_iMaxHp - m_iMaxHp / 8.f)
	{
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE2)] = true;
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE,DRAGON_PHASE::PHASE2);
	}
	else if (false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE3)] && m_iHp <= m_iMaxHp / 2.f)
	{
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE3)] = true;
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE3);
	}
}
void CEnderDragon::Flag_Check(_float fTimeDelta)
{
	//드래곤은 개별로 제어해서 체크해야겠네
	//사실 필요 없을지도~
	//플래그 왜만든거지
	//아아..
}
void CEnderDragon::Update_BBToFsm()
{
	auto pBB = Get_BlackBoard();

	if (nullptr == pBB)
		return;

	pBB->Set_Value(EDG_KEY::STATE, m_pFsm->GetCurState());
}
_bool CEnderDragon::BreakSkillType(PLAYER_SKILL_TYPE eType)
{
	uint32_t iSkillNumber = Find_SkillNum(m_eAttType);
	//파훼 됨?
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ACCIO:
		if (ETOUI(DRAGON_SKILL::FIREBALL) == iSkillNumber)
			return true;
		break;
	case PLAYER_SKILL_TYPE::DEPULSO:
		break;

	case PLAYER_SKILL_TYPE::DESCENDO:
		break;

	case PLAYER_SKILL_TYPE::ACIENT_LIGHTNING:
		break;
	}
	return false;
}
E::UPtr<CEnderDragon> CEnderDragon::Create()
{
	auto pInstance = E::ToUPtr(new CEnderDragon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEnderDragon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CEnderDragon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CEnderDragon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEnderDragon");
		return nullptr;
	}

	return pInstance;
}
