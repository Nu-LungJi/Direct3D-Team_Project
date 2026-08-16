#include "pch.h"
#include "Spider_Hit.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
NS_USING(Client)
CSpider_Hit::CSpider_Hit()
{
}

CSpider_Hit::~CSpider_Hit()
{
}
HRESULT CSpider_Hit::Initialize(const _string& strLevelTag, CSpider* pSpider)
{
	//이게맞나..
	//NORMAL
	{
		//Start
		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::NORMAL)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Hitch_Bwd_Add_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::AIR)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_PopUp_2Spin_Bwd_01_anm.bin"),.fBlend = 0.1f });
	}
	
	////////////////////ACCIOSIBAL///////////////////////
	{
	//Start
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::AIR)].push_back(MON_ANIM_FSM{.iAnimIndex =
			pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Levitate_Start_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::AIR)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Levitate_Loop_anm.bin"),.fBlend = 0.1f });

	//END
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::FALLING)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Spawn_Fall_E_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Land_Stagger_Land_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Land_Stagger_Hold_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
			pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Land_Stagger_GetUp_anm.bin"),.fBlend = 0.1f });

	}
	
	
	return S_OK;
}
void CSpider_Hit::Enter(CStateMachine* pStateMachine)
{
	CSpider* pSpider = pStateMachine->GetOwner<CSpider>();

	if (nullptr == pSpider)
		return;

	if (!pSpider->Activate_PendingHit())
		return;
	MON_HIT_INFO Pending = pSpider->Get_ActiveHitInfo();
	Reactive_Table(Pending.eHitType);


	m_HitStep = HIT_STEP::START;
}

void CSpider_Hit::Exit(CStateMachine* pStateMachine)
{
	auto pSpider = pStateMachine->GetOwner<CSpider>();
	if (nullptr == pSpider) return;


}

void CSpider_Hit::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pSpiderFsm = Cast<CSpider_State>(pStateMachine);
	if (nullptr == pSpiderFsm) return;

	auto pSpider = pStateMachine->GetOwner<CSpider>();
	if (nullptr == pSpider) return;

	auto pBB = pSpider->Get_BlackBoard();
	if (nullptr == pBB) return;

	Check_PendingHit(pSpider);
	switch (m_HitStep)
	{
	case HIT_STEP::START:
		Hit_Step_Start(pSpider);
		break;
	case HIT_STEP::LOOP:
		Hit_Step_Loop(pSpider);
		break;
	case HIT_STEP::END:
		Hit_Step_End(pSpider);
		break;

	}

}

void CSpider_Hit::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{



}

HIT_TYPE CSpider_Hit::Reactive_Table(PLAYER_SKILL_TYPE eType)
{

	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ATTACK:
		m_HitTable.eHitMotion = HIT_MOTION::NORMAL;
		return HIT_TYPE::NORMAL;
	case PLAYER_SKILL_TYPE::ACCIO:
		m_HitTable.eHitMotion = HIT_MOTION::AIR;
		return HIT_TYPE::LAUNCH;
	case PLAYER_SKILL_TYPE::DEPULSO:
		m_HitTable.eHitMotion = HIT_MOTION::BLOWBACK;
		return HIT_TYPE::KNOCKBACK;
	case PLAYER_SKILL_TYPE::DESCENDO:
		m_HitTable.eHitMotion = HIT_MOTION::GROUND_SLAM;
		return HIT_TYPE::SLAM;
	case PLAYER_SKILL_TYPE::PROTEGO:
		break;

	case PLAYER_SKILL_TYPE::ACIENT_LIGHTNING:
		break;
	}

	return HIT_TYPE::END;
}

void CSpider_Hit::Check_PendingHit(CSpider* pSpider)
{
	if (!pSpider->Is_PendingHit()) return;

	MON_HIT_INFO HitInfo = pSpider->Get_PendingHitInfo();
	
}

void CSpider_Hit::Hit_Step_Start(CSpider* pSpider)
{
}

void CSpider_Hit::Hit_Step_Loop(CSpider* pSpider)
{
}

void CSpider_Hit::Hit_Step_End(CSpider* pSpider)
{
}

void CSpider_Hit::Check_Loop(CComAnimator* pAnimator)
{
}

_bool CSpider_Hit::PlayAnim(CComAnimator* pAnimator, _bool bLoop)
{
	if (nullptr == pAnimator) return true;

	if (m_iAnimIndex == m_Anims[ETOUI(m_HitTable.eHitType)][ETOUI(m_HitTable.eHitMotion)].size()) return true;

	MON_ANIM_FSM pAnim = m_Anims[ETOUI(m_HitTable.eHitType)][ETOUI(m_HitTable.eHitMotion)][m_iAnimIndex];
	
	pAnimator->Play_Anim(pAnim.fBlend, false, pAnim.fBlend);
	
	if (!bLoop && pAnimator->GetFinish())
		++m_iAnimIndex;

}

SPtr<CSpider_Hit> CSpider_Hit::Create(const _string& strLevelTag, CSpider* pSpider)
{
	auto pInstance = ToSPtr(new CSpider_Hit{});
	if (FAILED(pInstance->Initialize(strLevelTag, pSpider)))
	{
		MSG_BOX("Failed to create CSpider_Hit");
		return nullptr;
	}

	return pInstance;
}
