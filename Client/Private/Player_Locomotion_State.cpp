#include "pch.h"
#include "Player_Locomotion_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)


CPlayer_Locomotion_State::CPlayer_Locomotion_State()
{

}

void CPlayer_Locomotion_State::Enter(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	m_iIdleAnimation = FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_anm.bin");
	m_LeftIdleTurns = {
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_Turn_Lft_45_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_Turn_Lft_90_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_Turn_Lft_135_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_Turn_Lft_180_anm.bin")
	};
	m_RightIdleTurns = {
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_Turn_Rht_45_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_Turn_Rht_90_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_Turn_Rht_135_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_Turn_Rht_180_anm.bin")
	};
	m_LeftJogTurns = {
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_AD_BM_JogFwdTurn45_L_RU_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_AD_BM_JogFwdTurn90_L_RU_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_AD_BM_JogFwdTurn135_L_RU_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_AD_BM_Jog_FwdTurn180_L_RU_anm.bin")
	};
	m_RightJogTurns = {
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_AD_BM_JogFwdTurn45_R_RU_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_AD_BM_JogFwdTurn90_R_RU_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_AD_BM_JogFwdTurn135_R_RU_anm.bin"),
		FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_AD_BM_Jog_FwdTurn180_R_RU_anm.bin")
	};
	m_iJogStartForwardAnimation = FindAnimationIndex(
		*player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_RF_Jog_Turn_Start_Fwd_RU_anm.bin");
	m_iJogForwardAnimation = FindAnimationIndex(
		*player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Jog_Loop_Fwd_anm.bin");
	m_iJogStopForwardAnimation = FindAnimationIndex(
		*player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_RF_Jog_Turn_Stop_Fwd_RU_anm.bin");
	m_iSprintForwardAnimation = FindAnimationIndex(
		*player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Sprint_Loop_Fwd_anm.bin");
	m_iSprintLeanLeftAnimation = FindAnimationIndex(
		*player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Sprint_Loop_Lean_Lft_anm.bin");
	m_iSprintLeanRightAnimation = FindAnimationIndex(
		*player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Sprint_Loop_Lean_Rht_anm.bin");

	if (auto* pAnimator = player->GetAnimator();
		pAnimator && m_iIdleAnimation >= 0)
	{
		pAnimator->Play_Anim(m_iIdleAnimation, true, 0.f);
	}
}

void CPlayer_Locomotion_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	auto* pMoveIntent = player->GetMoveIntent();
	auto* pAnimator = player->GetAnimator();
	if (!pMoveIntent || !pAnimator)
		return;

	if (m_bTurnPending)
	{
		pMoveIntent->ClearMoveIntent();
		pMoveIntent->ClearFacingIntent();

		if (!player->HasRawMoveInput())
		{
			m_bTurnPending = false;
			BeginIdleTurn(
				*player,
				m_vTurnTargetDirection,
				m_iPendingIdleTurnAnimation);
			return;
		}

		m_fTurnHoldTime += fTimeDelta;
		if (m_fTurnHoldTime >= m_fJogTurnHoldThreshold)
		{
			const _float fTurnAngle =
				CalculateSignedAngle(*player, m_vTurnTargetDirection);
			const int32_t iJogTurnAnimation =
				ResolveJogTurnAnimation(fTurnAngle);

			m_bTurnPending = false;
			if (iJogTurnAnimation >= 0)
			{
				BeginJogTurn(
					*player,
					m_vTurnTargetDirection,
					iJogTurnAnimation);
			}
			else
			{
				BeginIdleTurn(
					*player,
					m_vTurnTargetDirection,
					m_iPendingIdleTurnAnimation);
			}
		}

		return;
	}

	if (m_bIdleTurning)
	{
		pMoveIntent->ClearMoveIntent();
		pMoveIntent->ClearFacingIntent();
		UpdateIdleTurnRotation(*player, pAnimator->GetPlayAnimRatio());

		if (pAnimator->GetFinish())
			FinishIdleTurn(*player);

		return;
	}

	if (m_bJogStarting)
	{
		if (!player->HasRawMoveInput())
		{
			BeginJogStop(*player);
			return;
		}

		if (pAnimator->GetFinish())
		{
			m_bJogStarting = false;
			m_bWasMoving = true;
			player->SetRootMotionTranslationActive(false);
			player->SetMovementLocked(false);
			player->SetCurrentMoveSpeed(5.f);
			if (m_iJogForwardAnimation >= 0)
			{
				pAnimator->Play_Anim(
					m_iJogForwardAnimation,
					true,
					0.1f);
				m_iActiveMoveLoopAnimation = m_iJogForwardAnimation;
			}
		}

		pMoveIntent->SetFacingIntent(
			player->GetRawMoveDirection(),
			360.f);
		return;
	}

	if (m_bJogStopping)
	{
		if (player->HasRawMoveInput())
		{
			m_bJogStopping = false;
			m_bWasMoving = true;
			player->SetRootMotionTranslationActive(false);
			player->SetMovementLocked(false);
			player->SetCurrentMoveSpeed(2.5f);
			if (m_iJogForwardAnimation >= 0)
			{
				pAnimator->Play_Anim(
					m_iJogForwardAnimation,
					true,
					0.1f);
				m_iActiveMoveLoopAnimation = m_iJogForwardAnimation;
			}
			return;
		}

		if (pAnimator->GetFinish())
		{
			m_bJogStopping = false;
			m_bWasMoving = false;
			player->SetRootMotionTranslationActive(false);
			player->SetMovementLocked(false);
			player->SetCurrentMoveSpeed(0.f);
			pMoveIntent->ClearMoveIntent();
			if (m_iIdleAnimation >= 0)
				pAnimator->Play_Anim(
					m_iIdleAnimation,
					true,
					0.1f);
		}
		return;
	}

	const auto tMoveOutput = pMoveIntent->GetOutput();
	if (!player->HasRawMoveInput() &&
		m_bWasMoving &&
		player->GetCurrentMoveSpeed() > 0.f)
	{
		BeginJogStop(*player);
		return;
	}

	if (!tMoveOutput.bMoveRequested)
	{
		m_fSignedMoveAngle = 0.f;
		m_eMoveDirection = MOVE_DIRECTION::FRONT;
		m_bWasMoving = false;
		pMoveIntent->ClearFacingIntent();
		return;
	}

	// 플레이어가 아직 회전하기 전의 Look을 기준으로 애니메이션용 각도를 구한다.
	m_fSignedMoveAngle = CalculateSignedAngle(*player, tMoveOutput.vMoveDirection);
	m_eMoveDirection = ResolveDirection(m_fSignedMoveAngle);

	// 회전 보간 없이 입력 방향을 즉시 바라본다.
	const int32_t iTurnAnimation = m_bWasMoving? -1: ResolveIdleTurnAnimation(m_fSignedMoveAngle);
	if (iTurnAnimation >= 0)
	{
		BeginTurnDecision(*player, tMoveOutput.vMoveDirection, iTurnAnimation);
		return;
	}

	if (!m_bWasMoving)
	{
		BeginJogStart(*player);
		pMoveIntent->SetFacingIntent(
			tMoveOutput.vMoveDirection,
			360.f);
		return;
	}

	// 일반 이동에서는 입력 방향으로 순간이동하듯 꺾지 않고
	// 연속적인 월드 방향을 유지한 채 일정 회전속도로 따라간다.
	pMoveIntent->SetFacingIntent(tMoveOutput.vMoveDirection, 360.f);

	int32_t iDesiredMoveAnimation = m_iJogForwardAnimation;
	if (player->IsSprintRequested())
	{
		const _float fLeanAngle = CalculateSignedAngle(
			*player,
			player->GetRawMoveDirection());

		if (fLeanAngle >= 10.f && fLeanAngle <= 45.f)
			iDesiredMoveAnimation = m_iSprintLeanRightAnimation;
		else if (fLeanAngle <= -10.f && fLeanAngle >= -45.f)
			iDesiredMoveAnimation = m_iSprintLeanLeftAnimation;
		else
			iDesiredMoveAnimation = m_iSprintForwardAnimation;
	}

	if (iDesiredMoveAnimation >= 0 &&
		iDesiredMoveAnimation != m_iActiveMoveLoopAnimation)
	{
		pAnimator->Play_Anim(iDesiredMoveAnimation, true, 0.15f);
		m_iActiveMoveLoopAnimation = iDesiredMoveAnimation;
	}
	m_bWasMoving = true;
}

_float CPlayer_Locomotion_State::CalculateSignedAngle(const CPlayer& player,const _float3& vMoveDirection) const
{
	_vector vLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);

	_vector vRight = XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);

	_vector vMove = XMVectorSetY(XMLoadFloat3(&vMoveDirection), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vMove))<= std::numeric_limits<_float>::epsilon())
	{
		return 0.f;
	}

	vLook = XMVector3Normalize(vLook);
	vRight = XMVector3Normalize(vRight);
	vMove = XMVector3Normalize(vMove);

	const _float fForward = XMVectorGetX(XMVector3Dot(vMove, vLook));

	const _float fRight = XMVectorGetX(XMVector3Dot(vMove, vRight));


	return XMConvertToDegrees(std::atan2(fRight, fForward));
}

CPlayer_Locomotion_State::MOVE_DIRECTION
CPlayer_Locomotion_State::ResolveDirection(_float fSignedAngle) const
{
	// 각 방향의 중심에서 절반인 22.5도를 경계로 8방향을 나눈다.
	if (fSignedAngle < -157.5f || fSignedAngle >= 157.5f)
		return MOVE_DIRECTION::BACKWARD;
	if (fSignedAngle < -112.5f)
		return MOVE_DIRECTION::LEFT_135;
	if (fSignedAngle < -67.5f)
		return MOVE_DIRECTION::LEFT_90;
	if (fSignedAngle < -22.5f)
		return MOVE_DIRECTION::LEFT_45;
	if (fSignedAngle < 22.5f)
		return MOVE_DIRECTION::FRONT;
	if (fSignedAngle < 67.5f)
		return MOVE_DIRECTION::RIGHT_45;
	if (fSignedAngle < 112.5f)
		return MOVE_DIRECTION::RIGHT_90;
	return MOVE_DIRECTION::RIGHT_135;
}

int32_t CPlayer_Locomotion_State::FindAnimationIndex(const CPlayer& player,const _string_view& sAnimationName) const
{
	auto* pModelInstance = player.GetModelInstance();
	if (!pModelInstance || !pModelInstance->GetModel())
		return -1;

	const auto& Animations = pModelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < Animations.size(); ++i)
	{
		if (Animations[i] && Animations[i]->GetAnimName() == sAnimationName)
			return static_cast<int32_t>(i);
	}

	return -1;
}

int32_t CPlayer_Locomotion_State::ResolveIdleTurnAnimation(
	_float fSignedAngle) const
{
	const _float fAbsoluteAngle = std::abs(fSignedAngle);
	if (fAbsoluteAngle < 22.5f)
		return -1;

	size_t iAngleIndex{};
	if (fAbsoluteAngle < 67.5f)
		iAngleIndex = 0;
	else if (fAbsoluteAngle < 112.5f)
		iAngleIndex = 1;
	else if (fAbsoluteAngle < 157.5f)
		iAngleIndex = 2;
	else
		iAngleIndex = 3;

	return fSignedAngle < 0.f
		? m_LeftIdleTurns[iAngleIndex]
		: m_RightIdleTurns[iAngleIndex];
}

int32_t CPlayer_Locomotion_State::ResolveJogTurnAnimation(
	_float fSignedAngle) const
{
	const _float fAbsoluteAngle = std::abs(fSignedAngle);
	if (fAbsoluteAngle < 22.5f)
		return -1;

	size_t iAngleIndex{};
	if (fAbsoluteAngle < 67.5f)
		iAngleIndex = 0;
	else if (fAbsoluteAngle < 112.5f)
		iAngleIndex = 1;
	else if (fAbsoluteAngle < 157.5f)
		iAngleIndex = 2;
	else
		iAngleIndex = 3;

	return fSignedAngle < 0.f
		? m_LeftJogTurns[iAngleIndex]
		: m_RightJogTurns[iAngleIndex];
}

void CPlayer_Locomotion_State::BeginTurnDecision(
	CPlayer& player,
	const _float3& vTargetDirection,
	int32_t iIdleAnimation)
{
	m_bTurnPending = true;
	m_fTurnHoldTime = 0.f;
	m_iPendingIdleTurnAnimation = iIdleAnimation;
	m_vTurnTargetDirection = vTargetDirection;

	player.SetMovementLocked(true);
	if (auto* pMoveIntent = player.GetMoveIntent())
	{
		pMoveIntent->ClearMoveIntent();
		pMoveIntent->ClearFacingIntent();
	}
}

void CPlayer_Locomotion_State::BeginIdleTurn(
	CPlayer& player,
	const _float3& vTargetDirection,
	int32_t iAnimationIndex)
{
	auto* pAnimator = player.GetAnimator();
	auto* pMoveIntent = player.GetMoveIntent();
	if (!pAnimator || !pMoveIntent || iAnimationIndex < 0)
		return;

	m_bIdleTurning = true;
	m_bJogTurning = false;
	m_vTurnTargetDirection = vTargetDirection;

	player.SetMovementLocked(true);
	player.SetRootMotionRotationActive(false);
	player.SetRootMotionTranslationActive(true);
	pMoveIntent->ClearMoveIntent();
	pMoveIntent->ClearFacingIntent();

	XMStoreFloat4(
		&m_qTurnStartRotation,
		player.GetTransform().GetLoadedQuaternion());
	m_fTurnSignedAngleRadians = XMConvertToRadians(
		CalculateSignedAngle(player, vTargetDirection));

	const _float fTargetYaw = std::atan2(
		vTargetDirection.x,
		vTargetDirection.z);
	XMStoreFloat4(
		&m_qTurnTargetRotation,
		XMQuaternionRotationAxis(
			XMVectorSet(0.f, 1.f, 0.f, 0.f),
			fTargetYaw));

	pAnimator->Play_Anim(iAnimationIndex, false, 0.1f);
}

void CPlayer_Locomotion_State::BeginJogTurn(
	CPlayer& player,
	const _float3& vTargetDirection,
	int32_t iAnimationIndex)
{
	BeginIdleTurn(player, vTargetDirection, iAnimationIndex);
	m_bJogTurning = true;
}

void CPlayer_Locomotion_State::BeginJogStart(CPlayer& player)
{
	auto* pAnimator = player.GetAnimator();
	if (!pAnimator)
		return;

	m_bJogStarting = true;
	m_bJogStopping = false;
	m_bWasMoving = true;
	player.SetMovementLocked(true);
	player.SetRootMotionTranslationActive(true);

	if (m_iJogStartForwardAnimation >= 0)
	{
		pAnimator->Play_Anim(
			m_iJogStartForwardAnimation,
			false,
			0.1f);
		m_iActiveMoveLoopAnimation = -1;
	}
	else if (m_iJogForwardAnimation >= 0)
	{
		m_bJogStarting = false;
		player.SetRootMotionTranslationActive(false);
		player.SetMovementLocked(false);
		pAnimator->Play_Anim(
			m_iJogForwardAnimation,
			true,
			0.1f);
		m_iActiveMoveLoopAnimation = m_iJogForwardAnimation;
	}
}

void CPlayer_Locomotion_State::BeginJogStop(CPlayer& player)
{
	auto* pAnimator = player.GetAnimator();
	if (!pAnimator)
		return;

	m_bJogStarting = false;
	m_bJogStopping = true;
	player.SetMovementLocked(true);
	player.SetRootMotionTranslationActive(true);

	if (m_iJogStopForwardAnimation >= 0)
	{
		pAnimator->Play_Anim(
			m_iJogStopForwardAnimation,
			false,
			0.1f);
		m_iActiveMoveLoopAnimation = -1;
	}
	else
	{
		m_bJogStopping = false;
		m_bWasMoving = false;
		player.SetRootMotionTranslationActive(false);
		player.SetMovementLocked(false);
		player.SetCurrentMoveSpeed(0.f);
		if (m_iIdleAnimation >= 0)
			pAnimator->Play_Anim(
				m_iIdleAnimation,
				true,
				0.1f);
	}
}

void CPlayer_Locomotion_State::UpdateIdleTurnRotation(
	CPlayer& player,
	_float fAnimationRatio)
{
	const _float fRatio = std::clamp(fAnimationRatio, 0.f, 1.f);

	// 애니메이션 마지막 10%는 발 정리 구간으로 남기고,
	// 실제 회전은 SmoothStep으로 천천히 시작하고 천천히 끝낸다.
	const _float fNormalizedTurnRatio =
		std::clamp(fRatio / 0.9f, 0.f, 1.f);
	const _float fTurnRatio =
		fNormalizedTurnRatio *
		fNormalizedTurnRatio *
		(3.f - 2.f * fNormalizedTurnRatio);

	const _vector qTurnDelta = XMQuaternionRotationAxis(
		XMVectorSet(0.f, 1.f, 0.f, 0.f),
		m_fTurnSignedAngleRadians * fTurnRatio);

	player.GetTransform().SetQuaternion(
		XMQuaternionNormalize(
			XMQuaternionMultiply(
				XMLoadFloat4(&m_qTurnStartRotation),
				qTurnDelta)));
}

void CPlayer_Locomotion_State::FinishIdleTurn(CPlayer& player)
{
	auto* pAnimator = player.GetAnimator();
	auto* pMoveIntent = player.GetMoveIntent();

	player.SetRootMotionRotationActive(false);
	player.SetRootMotionTranslationActive(false);
	player.SetMovementLocked(false);
	player.GetTransform().SetQuaternion(
		XMLoadFloat4(&m_qTurnTargetRotation));

	const _bool bContinueJog =
		m_bJogTurning && player.HasRawMoveInput();

	if (pMoveIntent)
	{
		if (bContinueJog)
		{
			player.SetCurrentMoveSpeed(5.f);
			pMoveIntent->SetMoveIntent(
				player.GetRawMoveDirection(),
				player.GetCurrentMoveSpeed());
			pMoveIntent->SetFacingIntent(
				player.GetRawMoveDirection(),
				360.f);
		}
		else
		{
			pMoveIntent->ClearMoveIntent();
			pMoveIntent->SetFacingIntentImmediate(
				m_vTurnTargetDirection);
		}
	}

	if (pAnimator)
	{
		const int32_t iNextAnimation = bContinueJog
			? m_iJogForwardAnimation
			: m_iIdleAnimation;
		if (iNextAnimation >= 0)
			pAnimator->Play_Anim(iNextAnimation, true, 0.1f);
	}

	m_bIdleTurning = false;
	m_bJogTurning = false;
	m_bWasMoving = bContinueJog;
	m_vTurnTargetDirection = {};
	m_fTurnSignedAngleRadians = 0.f;
	m_iPendingIdleTurnAnimation = -1;
}

SPtr<CPlayer_Locomotion_State> CPlayer_Locomotion_State::Create()
{
	return ToSPtr(new CPlayer_Locomotion_State{});
}
