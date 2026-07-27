#include "pch.h"
#include "Player_DashSkill_State.h"

#include "Player.h"
#include "ComCharacterMoveIntent.h"

#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"
NS_USING(Client)

void CPlayer_DashSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);
	if (m_iDashAnimIndex < 0 || m_iDashEndAnimIndex < 0)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	_vector vDashDirection = pPlayer->HasRawMoveInput() ? XMLoadFloat3(&pPlayer->GetRawMoveDirection())
			: XMVectorSetY(pPlayer->GetTransform().GetState(STATE::LOOK),0.f);

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;

	}
	if (XMVectorGetX(XMVector3LengthSq(vDashDirection)) <=std::numeric_limits<_float>::epsilon())
	{
		vDashDirection = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	}

	XMStoreFloat3(&m_vDashDirection,XMVector3Normalize(vDashDirection));

	SetSkillControl( *pPlayer, true, false, false);

	if (auto* pMoveIntent = pPlayer->GetMoveIntent())
		pMoveIntent->SetFacingIntentImmediate(m_vDashDirection);

	pPlayer->SetCurrentMoveSpeed(0.f);

	pAnimator->Play_Anim(m_iDashAnimIndex,false, 0.1f);  


	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fScaleTime = 0.f;
	m_fDashElapsed = 0.f;
}


void CPlayer_DashSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	m_iDashAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink_2Cmbt_anm.bin");
	//m_iDashAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink_anm.bin");
	//m_iDashAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink1_anm.bin");
	//m_iDashAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink1_Loop_anm.bin");
	m_iDashEndAnimIndex = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Blink1_anm.bin");

	m_bAnimationIndicesCached =
		m_iDashAnimIndex >= 0 &&
		m_iDashEndAnimIndex >= 0;

}
void CPlayer_DashSkill_State::Update(CStateMachine* pStateMachine,_float fTimeDelta)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio =
		PlayerAnimationRatioGuard::Sanitize(
			pAnimator->GetPlayAnimRatio());

	m_fScaleTime += fTimeDelta;

	const float fDuration = 0.3f; 
	float fRatio = std::clamp(m_fScaleTime / fDuration, 0.f, 1.f);

	_float3 vScale;


	switch (m_ePhase)
	{
	case PHASE::CAST:
		if (m_fAnimRatio >= CAST_START_RATIO)
		{
			// DASH 시작 ---------------------------------------------------------------------------------------------------
			m_ePhase = PHASE::DASH;    

		}
		break;

	case PHASE::DASH:
	
		if (fRatio <= 1.f)
		{
			vScale.x = std::lerp(vNormalScale.x, vSmallScale.x, fRatio);
			vScale.y = std::lerp(vNormalScale.y, vSmallScale.y, fRatio);
			vScale.z = std::lerp(vNormalScale.z, vSmallScale.z, fRatio);
			pPlayer->GetTransform().SetScale(vScale);
		}
	
		if (pPlayer->HasRawMoveInput())
		{
			_vector vDashDirection = XMLoadFloat3(&pPlayer->GetRawMoveDirection());
			vDashDirection = XMVectorSetY(vDashDirection, 0.f);

			if (XMVectorGetX(XMVector3LengthSq(vDashDirection)) >std::numeric_limits<_float>::epsilon())
			{
				XMStoreFloat3(&m_vDashDirection,XMVector3Normalize(vDashDirection));

				if (auto* pMoveIntent = pPlayer->GetMoveIntent())
				{
					pMoveIntent->SetFacingIntentImmediate(m_vDashDirection);
				}
			}
		}

		{
			const _float fRemainingTime = std::max(0.f, DASH_DURATION - m_fDashElapsed);
			const _float fMoveTime = std::min(fTimeDelta, fRemainingTime);

			if (fMoveTime > 0.f)
			{
				pPlayer->ApplyDirectionalMovement(
					m_vDashDirection,
					DASH_SPEED,
					fMoveTime);
				m_fDashElapsed += fMoveTime;
			}
		}

		if (m_fDashElapsed >= DASH_DURATION)
		{
			// DASH 끝---------------------------------------------------------------------------------------------------
			m_ePhase = PHASE::RECOVERY;   
			pPlayer->PrepareLocomotionResume();
			ResetSkillControl(*pPlayer);
			pAnimator->Play_Anim(m_iDashEndAnimIndex, false, 0.24f);
			pAnimator->GetCurAnimState().fSpeed = 2.f;
			m_fScaleTime = 0.f;
		}
		break;

	case PHASE::RECOVERY:
		if (fRatio <= 1.f)
		{
			vScale.x = std::lerp(vSmallScale.x, vNormalScale.x, fRatio);
			vScale.y = std::lerp(vSmallScale.y, vNormalScale.y, fRatio);
			vScale.z = std::lerp(vSmallScale.z, vNormalScale.z, fRatio);
			pPlayer->GetTransform().SetScale(vScale);
		}

		if (pPlayer->HasRawMoveInput())
		{
			_vector vRecoveryDirection = XMLoadFloat3(&pPlayer->GetRawMoveDirection());
			vRecoveryDirection = XMVectorSetY(vRecoveryDirection, 0.f);

			if (XMVectorGetX(XMVector3LengthSq(vRecoveryDirection)) >std::numeric_limits<_float>::epsilon())
			{
				XMStoreFloat3(&m_vDashDirection,XMVector3Normalize(vRecoveryDirection));

				if (auto* pMoveIntent = pPlayer->GetMoveIntent())
				{
					pMoveIntent->SetFacingIntent(m_vDashDirection, 360.f);
				}
			}
		}

		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO)
		{
			pPlayer->GetTransform().SetScale(vNormalScale);
			RequestLocomotion(pStateMachine);
		}
		break;
	}
}

void CPlayer_DashSkill_State::Exit(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (pPlayer)
	{
		ResetSkillControl(*pPlayer);
		pPlayer->GetTransform().SetScale(_float3{ 1.f, 1.f, 1.f });
	}

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_vDashDirection = {};
	m_fScaleTime = 0.f;
	m_fDashElapsed = 0.f;
}

SPtr<CPlayer_DashSkill_State> CPlayer_DashSkill_State::Create()
{
	return ToSPtr(new CPlayer_DashSkill_State{});
}
