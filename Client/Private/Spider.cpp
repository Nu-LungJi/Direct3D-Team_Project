#include "pch.h"
#include "Spider.h"
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
#include "Spider_Spawn.h"
#include "Spider_Combat.h"
#include "Spider_Hit.h"
#include "Mon_Dead.h"
#include "Mon_Godae.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
NS_USING(Client)

CSpider::CSpider()
{
}


CSpider::~CSpider()
{
}

void CSpider::UpdateGUI()
{
	__super::UpdateGUI();
	
}

HRESULT CSpider::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}
	
	return S_OK;
}

HRESULT CSpider::Initialize(void* pArg)
{
	auto MonDesc = static_cast<SPIDER_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 35;
	m_fDissolve = 0.f;
	
	if (FAILED(Ready_Fsm(MonDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Fsm");
		return E_FAIL;
	}
	//if (FAILED(Ready_Skill(MonDesc->LevelTag)))
	//{
	//	MSG_BOX("Create Failed Skill");
	//	return E_FAIL;
	//}
	Ready_BBKeyValue();
	GetTransform().SetScale(XMLoadFloat3(&MonDesc->vScale));
	GetTransform().SetPosition(XMLoadFloat3(&MonDesc->vPos));
	m_eMonType = MONSTER_TYPE::NORMAL;
	
	m_pComSphereCol->SetQueryEnabled(true);
	m_iColliderBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("RigPelvisSocket");
	m_pModelAnimator->Play_Anim(0, false);
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);

	auto pBB = Get_BlackBoard();

	pBB->Set_Value<_float3>(NPC_KEY::STARTPOS, MonDesc->vPatrollStart);
	pBB->Set_Value<_float3>(NPC_KEY::ENDPOS, MonDesc->vPatrollEnd);
	return S_OK;
}
HRESULT CSpider::Ready_Fsm(const _string& LevelTag)
{
	CMon_State::DESC Desc{};
	if (FAILED(AddComponentFromProto(LevelTag, "Prototype_Component_Mon_FSM", "Mon_Fsm", &Desc, &m_pFsm))) return E_FAIL;
	
	
	if (false == m_pFsm->Add_State(MON_STATE::SPAWN, CSpider_Spawn::Create(LevelTag))) return E_FAIL;
	if (false == m_pFsm->Add_State(MON_STATE::COMBAT, CSpider_Combat::Create(LevelTag))) return E_FAIL;
	if (false == m_pFsm->Add_State(MON_STATE::HIT, CSpider_Hit::Create(LevelTag,this))) return E_FAIL;
	if (false == m_pFsm->Add_State(MON_STATE::DEAD, CMon_Dead::Create("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Death_v04_anm.bin",this))) return E_FAIL;
	if (false == m_pFsm->Add_State(MON_STATE::GODAE, CMon_Godae::Create("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_BM_Idle_anm.bin", this))) return E_FAIL;

	if (false == m_pFsm->Initialize_State(MON_STATE::SPAWN)) return E_FAIL;


	return S_OK;
}
HRESULT CSpider::Ready_Skill(const _string& LevelTag)
{

	return S_OK;
}
void CSpider::Ready_BBKeyValue()
{
	auto pBB = Get_BlackBoard();

	pBB->Set_Value<CHandle>(PUBLIC_KEY::TARGETHANDLE, m_TargetHandle);
}
void CSpider::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_iHp <= 0.f || m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DEAD)))
		m_pFsm->Request_State(MON_STATE::DEAD);
	
	if (!m_bSpawn) return;
	if (m_bEndGame)
	{
		m_fDissolve = 0;
		SetPendingDestroy();
		return;
	}
	m_fTick += fTimeDelta;
	m_pFsm->PriorityUpdate(fTimeDelta);
	Update_BBToFsm();
	__super::PriorityUpdate(fTimeDelta);
	m_pFsm->Update(fTimeDelta);
}

void CSpider::Update(E::_float fTimeDelta)
{
	if (!m_bSpawn) return;
	if (m_bEndGame) return;
	__super::Update(fTimeDelta);
	if (m_fTick > 3.f)
	{
		Find_Target();
		m_fTick = 0.f;
	}
}

void CSpider::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bSpawn) return;
	if (m_bEndGame) return;
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CSpider::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bSpawn) return;
	m_pFsm->LateUpdate(fTimeDelta);
	__super::LateUpdate(fTimeDelta);

}

void CSpider::Set_StateFinished(_bool bFinished)
{
	//스테이트가 완료된 판정에 대해서 다시 초기회
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;

	pBB->Set_Value(EDG_KEY::BSTATE_FINISHED, bFinished);
}
_bool CSpider::Is_StateFinished()
{
	//스테이트가 끝났는지 확인
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return false;

	auto pbFinished = pBB->Get_Value<_bool>(EDG_KEY::BSTATE_FINISHED);
	if (nullptr == pbFinished) return false;

	return *pbFinished;
}
_string CSpider::Get_SkillName(ATTMON SkillNode)
{
	auto pValue = m_MonSkillLists.find(SkillNode);

	if (pValue == m_MonSkillLists.end())
		return "";

	if (pValue->second >= ETOUI(SPIDER_SKILL::END))
		return "";

	return MagicEnumToStringView(static_cast<SPIDER_SKILL>(pValue->second)).data();
}

_bool CSpider::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (eType == PLAYER_SKILL_TYPE::ANCIENT_LIGHTNING || eType == PLAYER_SKILL_TYPE::ABRA)
		m_iHp = 0;

	if (m_iHp <= 0.f || m_pFsm->GetCurState() == MON_STATE::DEAD)
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
	 
	m_PendingMonTable.eAttType = m_eAttType;
	m_PendingMonTable.eHitType = eType;
	
	m_bPending = true;
	m_pFsm->Request_State(MON_STATE::HIT);
	return true;

}

void CSpider::Set_AttTable(ATTMON eType, _float2 fSkillRatio)
{
	if (eType == ATTMON::END)
		return;

	uint32_t iSkillNum = Find_SkillNum(eType);
	if (iSkillNum == UINT_MAX || iSkillNum >= ETOUI(SPIDER_SKILL::END))
		return;


	m_CurEffectName.clear();
	m_eAttType = ATTMON::END;
	m_eLastSkillTable = m_eAttType = eType;

}
void CSpider::Flag_Check(_float fTimeDelta)
{
	
}
void CSpider::Set_Gravity(_bool bGravity)
{
		if (bGravity)
			m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
		else
			m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);
}
const _float CSpider::Get_Damage()
{
	return 5.f;
}
void CSpider::Update_BBToFsm()
{
	auto pBB = Get_BlackBoard();

	if (nullptr == pBB)
		return;

	pBB->Set_Value(EDG_KEY::STATE, m_pFsm->GetCurState());
}
_bool CSpider::BreakSkillType(PLAYER_SKILL_TYPE eType)
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

	case PLAYER_SKILL_TYPE::ANCIENT_LIGHTNING:
		break;
	case PLAYER_SKILL_TYPE::DESTORY:
		return true;
		break;
	}
	return false;
}

void CSpider::Stuck()
{
	if (nullptr != m_pFsm)
		m_pFsm->Request_State(MON_STATE::GODAE);
}

E::UPtr<CSpider> CSpider::Create()
{
	auto pInstance = E::ToUPtr(new CSpider{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CSpider");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CSpider::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CSpider{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CSpider");
		return nullptr;
	}

	return pInstance;
}
