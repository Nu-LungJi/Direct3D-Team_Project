#include "pch.h"
#include "Gur_Hit.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
#include "ComCharacterMotor.h"
NS_USING(Client)
CGur_Hit::CGur_Hit()
{
}

CGur_Hit::~CGur_Hit()
{
}
HRESULT CGur_Hit::Initialize(const _string& strLevelTag, CTmbGurdian* pTmb)
{
	//이게맞나..
	//NORMAL
	{
		//Start
		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::NORMAL)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Hit_Bck_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::AIR)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Bump_Spin_Fwd_01_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::FALLING)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Bump_Spin_Fwd_01_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Send_Bwd_GetUp_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::NORMAL)][ETOUI(HIT_MOTION::AIR_LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Send_Fwd_GetUp_anm.bin"),.fBlend = 0.1f });

	}

	////////////////////ACCIOSIBAL///////////////////////
	{
		//Start
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::AIR)].push_back(MON_ANIM_FSM{ .iAnimIndex =
			pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Accio_02_anm.bin"),.fBlend = 0.2f });
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::AIR)].push_back(MON_ANIM_FSM{ .iAnimIndex =
			pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Accio_01_anm.bin"),.fBlend = 0.1f });

		//END
		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::FALLING)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Levitated_Loop_01_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::LAUNCH)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
				pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Land_Stagger_GetUp_anm.bin"),.fBlend = 0.1f });

	}

	{
		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::GROUND_SLAM)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Descendo_Grnd_anm.bin"),.fBlend = 0.1f });


		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::AIR)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Send_Fwd_Loop_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::FALLING)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Send_Fwd_Loop_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::REBOUND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Slam_FU_BounceUp_Fwd_anm.bin"),.fBlend = 0.1f });


		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Send_Fwd_SplatHold_anm.bin"),.fBlend = 0.1f });
		m_Anims[ETOUI(HIT_TYPE::SLAM)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Send_Fwd_GetUp_anm.bin"),.fBlend = 0.1f });

	}

	{
		m_Anims[ETOUI(HIT_TYPE::KNOCKBACK)][ETOUI(HIT_MOTION::BLOWBACK)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Send_Bwd_Loop_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::KNOCKBACK)][ETOUI(HIT_MOTION::FALLING)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Send_Bwd_Loop_anm.bin"),.fBlend = 0.1f });

		m_Anims[ETOUI(HIT_TYPE::KNOCKBACK)][ETOUI(HIT_MOTION::LAND)].push_back(MON_ANIM_FSM{ .iAnimIndex =
		pTmb->Find_AnimIndex("AN_SK_GOL_TombProtectorGrunt_LOD0_Skeleton_TMBG_Rct_Send_Bwd_GetUp_anm.bin"),.fBlend = 0.1f });


	}

	return S_OK;
}
void CGur_Hit::Enter(CStateMachine* pStateMachine)
{
	CTmbGurdian* pTmb = pStateMachine->GetOwner<CTmbGurdian>();

	if (nullptr == pTmb)
		return;

	if (!pTmb->Activate_PendingHit())
		return;
	MON_HIT_INFO Pending = pTmb->Get_ActiveHitInfo();
	m_HitTable.eHitType = Reactive_TableMotion(Pending.eHitType, pTmb->Is_Grounded());
	m_HitTable.eSkillType = Pending.eHitType;

	m_iAnimIndex = 0;
}

void CGur_Hit::Exit(CStateMachine* pStateMachine)
{
	auto pTmb = pStateMachine->GetOwner<CTmbGurdian>();
	if (nullptr == pTmb) return;
	m_HitTable.eHitMotion = HIT_MOTION::END;
	pTmb->ReActiveTable();
	Set_Gravity(true, pTmb);
}

void CGur_Hit::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pTmbFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pTmbFsm) return;

	auto pTmb = pStateMachine->GetOwner<CTmbGurdian>();
	if (nullptr == pTmb) return;
	if (pTmb->Get_CurrentHp() <= 0)
		pTmbFsm->Request_State(MON_STATE::COMBAT);
	Check_PendingHit(pTmb);
}

void CGur_Hit::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pTmbFsm = Cast<CMon_State>(pStateMachine);
	if (nullptr == pTmbFsm) return;

	auto pTmb = pStateMachine->GetOwner<CTmbGurdian>();
	if (nullptr == pTmb) return;

	auto pAnimator = pTmb->Get_Animator();
	if (nullptr == pAnimator) return;

	auto pMove = pTmb->Get_MoveIntent();
	if (nullptr == pMove)return;
	if (m_bTurn)
	{
		if(m_HitTable.eSkillType != PLAYER_SKILL_TYPE::ATTACK)
			pMove->SetFacingIntentImmediate(TargetDir(pTmb, false));

		m_bTurn = false;
	}

	MotionToPlay(pTmb, pAnimator, pTmbFsm);


}

_float3 CGur_Hit::TargetDir(CTmbGurdian* pTmb, _bool bFront)
{
	auto pTarget = pTmb->Get_Target();
	if (nullptr == pTarget) return _float3(0, 0, 1);

	_float3 vTargetPos = pTarget->GetTransform().GetPosition();
	_float3 vSrcPos = pTmb->GetTransform().GetPosition();
	_float3 vDir{};
	if (bFront)
		XMStoreFloat3(&vDir, XMVector3Normalize(XMLoadFloat3(&vSrcPos) - XMLoadFloat3(&vTargetPos)));
	else
		XMStoreFloat3(&vDir, XMVector3Normalize(XMLoadFloat3(&vTargetPos) - XMLoadFloat3(&vSrcPos)));

	return vDir;
}

void CGur_Hit::MoveIntent(CTmbGurdian* pTmb, _float3 vDir, _float fSpeed)
{
	auto pMove = pTmb->Get_MoveIntent();
	if (nullptr == pMove) return;

	pMove->SetMoveIntent(vDir, fSpeed);
}


HIT_TYPE CGur_Hit::Reactive_TableMotion(PLAYER_SKILL_TYPE eType, _bool bIsGround)
{

	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ATTACK:
		if (m_HitTable.eHitMotion == HIT_MOTION::LAND)
			m_HitTable.eHitMotion = HIT_MOTION::LAND;
		else if (m_HitTable.eHitMotion == HIT_MOTION::AIR || m_HitTable.eHitMotion == HIT_MOTION::REBOUND || m_HitTable.eHitMotion == HIT_MOTION::BLOWBACK)
			m_HitTable.eHitMotion = HIT_MOTION::AIR;
		else
			m_HitTable.eHitMotion = HIT_MOTION::NORMAL;
		return HIT_TYPE::NORMAL;
	case PLAYER_SKILL_TYPE::ACCIO:
		m_HitTable.eHitMotion = HIT_MOTION::AIR;
		return HIT_TYPE::LAUNCH;
	case PLAYER_SKILL_TYPE::DEPULSO:
		m_HitTable.eHitMotion = HIT_MOTION::BLOWBACK;
		return HIT_TYPE::KNOCKBACK;
	case PLAYER_SKILL_TYPE::DESCENDO:
		if (m_HitTable.eHitMotion == HIT_MOTION::AIR)
			m_HitTable.eHitMotion = HIT_MOTION::AIR;
		else m_HitTable.eHitMotion = HIT_MOTION::GROUND_SLAM;
		return HIT_TYPE::SLAM;
	case PLAYER_SKILL_TYPE::PROTEGO:
		break;

	case PLAYER_SKILL_TYPE::ACIENT_LIGHTNING:
		break;
	}

	return HIT_TYPE::END;
}

void CGur_Hit::Check_PendingHit(CTmbGurdian* pTmb)
{
	if (!pTmb->Is_PendingHit()) return;

	auto pAnim = pTmb->Get_Animator();
	if (nullptr == pAnim) return;

	MON_HIT_INFO HitInfo = pTmb->Get_PendingHitInfo();

	if (!pTmb->Activate_PendingHit()) return;
	_bool bBlock = HitInfo.eHitType == PLAYER_SKILL_TYPE::ATTACK && (m_HitTable.eHitMotion == HIT_MOTION::LAND || m_HitTable.eHitMotion == HIT_MOTION::AIR_LAND
		|| m_HitTable.eHitMotion == HIT_MOTION::GROUND_SLAM);
	if (bBlock)
		return;

	_bool bRestart = HitInfo.eHitType == PLAYER_SKILL_TYPE::ATTACK;
	if (m_HitTable.eSkillType == HitInfo.eHitType && !bRestart)
		return;

	m_HitTable.eHitType = Reactive_TableMotion(HitInfo.eHitType, pTmb->Is_Grounded());


	m_HitTable.eSkillType = HitInfo.eHitType;

	auto& State = pAnim->GetCurAnimState();
	State.fTrackPosition = 0.f;
	m_iAnimIndex = 0;

	m_bTurn = true;
}

void CGur_Hit::MotionToPlay(CTmbGurdian* pTmb, CComAnimator* pAnimator, CMon_State* pTmbState)
{
	_bool bGround = pTmb->Is_Grounded();
	if (m_HitTable.eHitMotion == HIT_MOTION::AIR)
		Set_Gravity(false, pTmb);
	else
		Set_Gravity(true, pTmb);

	switch (m_HitTable.eHitType)
	{
	case HIT_TYPE::NORMAL:
	{
		Set_Gravity(true, pTmb);
		switch (m_HitTable.eHitMotion)
		{
		case HIT_MOTION::NORMAL:
			if (PlayAnim(pAnimator))
				Finishied(pTmbState);
			break;
		case HIT_MOTION::AIR:
			PlayAnim(pAnimator, true);
			if (bGround)
				ChangeMotion(HIT_MOTION::AIR_LAND);
			
			MoveIntent(pTmb, TargetDir(pTmb, true), 7.f);
			break;
		case HIT_MOTION::FALLING:
			PlayAnim(pAnimator, true);
			if (bGround)
				ChangeMotion(HIT_MOTION::AIR_LAND);
			break;
		case HIT_MOTION::LAND:
			if (PlayAnim(pAnimator, false))
				Finishied(pTmbState);
			break;
		case HIT_MOTION::GROUND_SLAM:
			ChangeMotion(HIT_MOTION::AIR);
			break;
		case HIT_MOTION::BLOWBACK:
			ChangeMotion(HIT_MOTION::AIR);
			break;
		case HIT_MOTION::AIR_LAND:
			if (PlayAnim(pAnimator, false))
				Finishied(pTmbState);
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
			PlayAnim(pAnimator, true);
			if (bGround)
				ChangeMotion(HIT_MOTION::LAND);
			break;
		case HIT_MOTION::LAND:
			if (PlayAnim(pAnimator))
				Finishied(pTmbState);
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
			break;
		case HIT_MOTION::FALLING:
			PlayAnim(pAnimator, true);
			if (bGround)
				ChangeMotion(HIT_MOTION::LAND);
			Jump(pTmb, -150.f, false);

			break;
		case HIT_MOTION::AIR:
			Set_Gravity(true, pTmb);
			PlayAnim(pAnimator, true);
			if (bGround)
				ChangeMotion(HIT_MOTION::REBOUND);
			Jump(pTmb, -150.f, false);
			break;
		case HIT_MOTION::LAND:
			if (PlayAnim(pAnimator))
				Finishied(pTmbState);
	
			break;
		case HIT_MOTION::REBOUND:
			if (PlayAnim(pAnimator, false))
				ChangeMotion(HIT_MOTION::FALLING);
			Jump(pTmb, 6.f, false);
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
			MoveIntent(pTmb, TargetDir(pTmb, true), 22.f);
			break;
		case HIT_MOTION::AIR:
			ChangeMotion(HIT_MOTION::BLOWBACK);
			break;
		case HIT_MOTION::FALLING:
			PlayAnim(pAnimator, true);
			if (bGround)
				ChangeMotion(HIT_MOTION::LAND);

			MoveIntent(pTmb, TargetDir(pTmb, true), 22.f);
			break;
		case HIT_MOTION::LAND:
			if (PlayAnim(pAnimator))
				Finishied(pTmbState);
			break;
		}
		break;
	}
	}
}


_bool CGur_Hit::PlayAnim(CComAnimator* pAnimator, _bool bLoop)
{
	if (nullptr == pAnimator) return true;

	if (m_iAnimIndex >= m_Anims[ETOUI(m_HitTable.eHitType)][ETOUI(m_HitTable.eHitMotion)].size()) return true;

	auto pAnim = m_Anims[ETOUI(m_HitTable.eHitType)][ETOUI(m_HitTable.eHitMotion)];

	pAnimator->Play_Anim(pAnim[m_iAnimIndex].iAnimIndex, bLoop, pAnim[m_iAnimIndex].fBlend);

	if (!bLoop && pAnimator->GetFinish())
		++m_iAnimIndex;

	return m_iAnimIndex >= pAnim.size();
}

void CGur_Hit::Finishied(CMon_State* pTmbState)
{
	pTmbState->Request_State(MON_STATE::COMBAT);
}

void CGur_Hit::ChangeMotion(HIT_MOTION eMotion)
{
	if (m_HitTable.eHitMotion == eMotion) return;

	m_HitTable.eHitMotion = eMotion;
	m_iAnimIndex = 0;
}
void				CGur_Hit::Set_Gravity(_bool bGravity, CTmbGurdian* pTmb)
{
	pTmb->Set_Gravity(bGravity);
}
void CGur_Hit::Jump(CTmbGurdian* pTmb, _float fPower, _bool bUp)
{
	auto pMove = pTmb->Get_MoveIntent();
	if (nullptr == pMove) return;
	auto pMotor = pTmb->GetComponent<CComCharacterMotor>("ComCharacterMotor");
	pMove->RequestJump();
	_float3 vVelocity = pMotor->GetVelocity();

	vVelocity.y = fPower;
	if (bUp)
		pMotor->SetVelocity(vVelocity);
	else
		pMotor->SetGravity(fPower);
}
SPtr<CGur_Hit> CGur_Hit::Create(const _string& strLevelTag, CTmbGurdian* pTmb)
{
	auto pInstance = ToSPtr(new CGur_Hit{});
	if (FAILED(pInstance->Initialize(strLevelTag, pTmb)))
	{
		MSG_BOX("Failed to create CGur_Hit");
		return nullptr;
	}

	return pInstance;
}
