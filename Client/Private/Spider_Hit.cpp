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
		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::FALLING)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Air_Loop_Fall_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Land_Stagger_Land_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Land_Stagger_Hold_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
			pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Land_Stagger_GetUp_anm.bin"),.fBlend = 0.1f });

	}
	
	////////////////////ACCIOSIBAL///////////////////////
	{
	//Start
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::AIR)].push_back(MON_ANIM_FSM{.iAnimIndex =
			pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Accio_Right_anm.bin"),.fBlend = 0.1f });
	
	//END
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::FALLING)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Air_Loop_Fall_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Land_Stagger_Land_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Land_Stagger_Hold_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
			pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Land_Stagger_GetUp_anm.bin"),.fBlend = 0.1f });

	}
	
	{
		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::GROUND_SLAM)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Slam_FD_BounceUp_Fwd_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::AIR)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Air_Descendo_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::FALLING)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Air_Loop_Fall_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::REBOUND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Slam_FD_BounceUp_Fwd_anm.bin"),.fBlend = 0.1f });


		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Descendo_Slam_Start_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Descendo_Slam_Loop_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Descendo_Slam_End_anm.bin"),.fBlend = 0.1f });
	
	}
	
	{
		m_Anims[ETOUI(HIT_TYPE::KNOCKBACK)][ETOUI(HIT_MOTION::BLOWBACK)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Bump_Spin_Bwd_01_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::KNOCKBACK)][ETOUI(HIT_MOTION::FALLING)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_Send_Bwd_Loop_v01_Fast_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::KNOCKBACK)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pSpider->Find_AnimIndex("AN_SK_Thornback_Spider_Biting_Master_LOD0_Skeleton_SPD_Rct_KnckDn_Bwd_Getup_Bck_02_anm.bin"),.fBlend = 0.1f });

		
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
	m_HitTable.eHitType = Reactive_TableMotion(Pending.eHitType);
	m_HitTable.eSkillType = Pending.eHitType;

	m_iAnimIndex = 0;
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

	Check_PendingHit(pSpider);
}

void CSpider_Hit::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pSpiderFsm = Cast<CSpider_State>(pStateMachine);
	if (nullptr == pSpiderFsm) return;

	auto pSpider = pStateMachine->GetOwner<CSpider>();
	if (nullptr == pSpider) return;

	auto pAnimator = pSpider->Get_Animator();
	if (nullptr == pAnimator) return;



	MotionToPlay(pSpider, pAnimator, pSpiderFsm);


}

_float3 CSpider_Hit::TargetDir(CSpider* pSpider, _bool bFront)
{
	auto pTarget = pSpider->Get_Target();
	if (nullptr == pTarget) return _float3(0,0,1);

	_float3 vTargetPos = pTarget->GetTransform().GetPosition();
	_float3 vSrcPos = pSpider->GetTransform().GetPosition();
	_float3 vDir{};
	if(bFront)
		XMStoreFloat3(&vDir, XMVector3Normalize(XMLoadFloat3(&vSrcPos) - XMLoadFloat3(&vTargetPos)));
	else
		XMStoreFloat3(&vDir, XMVector3Normalize(XMLoadFloat3(&vTargetPos) - XMLoadFloat3(&vSrcPos)));

	return vDir;
}

void CSpider_Hit::MoveIntent(CSpider* pSpider, _float3 vDir, _float fSpeed)
{
	auto pMove = pSpider->Get_MoveIntent();
	if (nullptr == pMove) return;

	pMove->SetMoveIntent(vDir, fSpeed);
}

HIT_TYPE CSpider_Hit::Reactive_OnlyType(PLAYER_SKILL_TYPE eType)
{
	
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ATTACK:
		return  HIT_TYPE::NORMAL;
	case PLAYER_SKILL_TYPE::ACCIO:
		return HIT_TYPE::LAUNCH;
	case PLAYER_SKILL_TYPE::DEPULSO:
		return HIT_TYPE::KNOCKBACK;
	case PLAYER_SKILL_TYPE::DESCENDO:
		return HIT_TYPE::SLAM;
	case PLAYER_SKILL_TYPE::PROTEGO:
		break;
	
	case PLAYER_SKILL_TYPE::ACIENT_LIGHTNING:
		break;
	}

	return HIT_TYPE::END;
}

HIT_TYPE CSpider_Hit::Reactive_TableMotion(PLAYER_SKILL_TYPE eType)
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

	auto pAnim = pSpider->Get_Animator();
	if (nullptr == pAnim) return;

	MON_HIT_INFO HitInfo = pSpider->Get_PendingHitInfo();

	if (!pSpider->Activate_PendingHit()) return;

	_bool bRestart = HitInfo.eHitType == PLAYER_SKILL_TYPE::ATTACK;

	_bool bNormalToSkill = m_HitTable.eSkillType == PLAYER_SKILL_TYPE::ATTACK &&
						   HitInfo.eHitType != PLAYER_SKILL_TYPE::ATTACK;

	if (m_HitTable.eSkillType == HitInfo.eHitType && !bRestart)
		return;
	if (bNormalToSkill)
	{
		m_HitTable.eHitType = Reactive_TableMotion(HitInfo.eHitType);
	}
	else
		m_HitTable.eHitType = Reactive_OnlyType(HitInfo.eHitType);
	
	m_HitTable.eSkillType = HitInfo.eHitType;

	auto& State = pAnim->GetCurAnimState();
	State.fTrackPosition = 0.f;
	m_iAnimIndex = 0;

}

void CSpider_Hit::MotionToPlay(CSpider* pSpider, CComAnimator* pAnimator, CSpider_State* pSpiderState)
{
	_bool bGround = pSpider->Is_Grounded();
	if(m_HitTable.eHitMotion == HIT_MOTION::AIR)
		Set_Gravity(false, pSpider);
	else 
		Set_Gravity(true, pSpider);

	switch (m_HitTable.eHitType)
	{
		case HIT_TYPE::NORMAL:
		{
			switch (m_HitTable.eHitMotion)
			{
			case HIT_MOTION::NORMAL:
				if (PlayAnim(pAnimator))
					Finishied(pSpiderState);
				MoveIntent(pSpider, TargetDir(pSpider, true), 1.f);
				break;
			case HIT_MOTION::AIR:
				if (PlayAnim(pAnimator))
				{
					if (!bGround)
						ChangeMotion(HIT_MOTION::FALLING);
					else
						ChangeMotion(HIT_MOTION::LAND);
				}

				MoveIntent(pSpider, TargetDir(pSpider,true), 7.f);
				break;
			case HIT_MOTION::FALLING:
				PlayAnim(pAnimator, true);
					if (bGround)
						ChangeMotion(HIT_MOTION::LAND);
				break;
			case HIT_MOTION::LAND:
				if (PlayAnim(pAnimator))
					Finishied(pSpiderState);
				break;
			}
			break;
		}
		case HIT_TYPE::LAUNCH:
		{
			switch (m_HitTable.eHitMotion)
			{
			case HIT_MOTION::AIR:
				if (PlayAnim(pAnimator))
					ChangeMotion(HIT_MOTION::FALLING);
				break;
			case HIT_MOTION::FALLING:
				PlayAnim(pAnimator,true);
				if (bGround)
					ChangeMotion(HIT_MOTION::LAND);
				break;
			case HIT_MOTION::LAND:
				if (PlayAnim(pAnimator))
					Finishied(pSpiderState);
				break;
			case HIT_MOTION::BLOWBACK:
				ChangeMotion(HIT_MOTION::FALLING);
			}
			break;
		}
		case HIT_TYPE::SLAM:
		{
			switch (m_HitTable.eHitMotion)
			{
			case HIT_MOTION::GROUND_SLAM:
				if (PlayAnim(pAnimator))
					ChangeMotion(HIT_MOTION::LAND);
				MoveIntent(pSpider, _float3(0.f,1.f,0.f), 5.f);
				break;
			case HIT_MOTION::FALLING:
				PlayAnim(pAnimator, true);
				if(bGround)
					ChangeMotion(HIT_MOTION::GROUND_SLAM);
				break;
			case HIT_MOTION::AIR:
					ChangeMotion(HIT_MOTION::FALLING);
				break;
			case HIT_MOTION::LAND:
				if (PlayAnim(pAnimator))
					 Finishied(pSpiderState);
				break;
			}
			break;
		}
		case HIT_TYPE::KNOCKBACK:
		{
			switch (m_HitTable.eHitMotion)
			{
			case HIT_MOTION::BLOWBACK:
				if (PlayAnim(pAnimator))
				{
					if (bGround)
						ChangeMotion(HIT_MOTION::LAND);
					else
						ChangeMotion(HIT_MOTION::FALLING);
				}
				MoveIntent(pSpider, TargetDir(pSpider, true), 13.f);
				break;
			case HIT_MOTION::AIR:
				ChangeMotion(HIT_MOTION::BLOWBACK);
				break;
			case HIT_MOTION::FALLING:
				PlayAnim(pAnimator, true);
				if (bGround)
					ChangeMotion(HIT_MOTION::LAND);
				break;
			case HIT_MOTION::LAND:
				if (PlayAnim(pAnimator))
					Finishied(pSpiderState);
				break;
			}
			break;
		}
	}
}


_bool CSpider_Hit::PlayAnim(CComAnimator* pAnimator, _bool bLoop)
{
	if (nullptr == pAnimator) return true;

	if (m_iAnimIndex >= m_Anims[ETOUI(m_HitTable.eHitType)][ETOUI(m_HitTable.eHitMotion)].size()) return true;

	auto pAnim = m_Anims[ETOUI(m_HitTable.eHitType)][ETOUI(m_HitTable.eHitMotion)];
	
	pAnimator->Play_Anim(pAnim[m_iAnimIndex].iAnimIndex, bLoop, pAnim[m_iAnimIndex].fBlend);
	
	if (!bLoop && pAnimator->GetFinish())
		++m_iAnimIndex;

	return m_iAnimIndex >= pAnim.size();
}

void CSpider_Hit::Finishied(CSpider_State* pSpiderState)
{
	pSpiderState->Request_State(MON_STATE::COMBAT);
}

void CSpider_Hit::ChangeMotion(HIT_MOTION eMotion)
{
	if (m_HitTable.eHitMotion == eMotion) return;

	m_HitTable.eHitMotion = eMotion;
	m_iAnimIndex = 0;
}
void				CSpider_Hit::Set_Gravity(_bool bGravity, CSpider* pSpider)
{
	pSpider->Set_Gravity(bGravity);
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
