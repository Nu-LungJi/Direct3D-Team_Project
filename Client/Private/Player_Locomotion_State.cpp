#include "pch.h"
#include "Player_Locomotion_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "PlayerAnimationRatioGuard.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
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

	m_bJogStarting = false;
	m_bJogStopping = false;
	m_bWasMoving = false;
	m_iActiveMoveLoopAnimation = -1;
	m_fAirborneTime = 0.f;
	player->SetMovementLocked(false);
	player->SetRootMotionRotationActive(false);
	player->SetRootMotionTranslationActive(false);

	CacheAnimationIndices(*player);

	if (auto* pAnimator = player->GetAnimator();
		pAnimator &&
		!player->HasRawMoveInput() &&
		m_iIdleAnimation >= 0)
	{
		pAnimator->Play_Anim(m_iIdleAnimation, true, 0.4f);
	}
}

void CPlayer_Locomotion_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	m_iIdleAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_anm.bin");
	m_iWalkForwardAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Walk_Loop_Fwd_anm.bin");
	m_iJogStartForwardAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_RF_Jog_Start_Fwd_anm.bin");
	m_iJogForwardAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Jog_Loop_Fwd_anm.bin");
	m_iJogStopForwardAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_RF_Jog_Turn_Stop_Fwd_RU_anm.bin");
	m_iSprintForwardAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Sprint_Loop_Fwd_anm.bin");
	m_iSprintLeanLeftAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Sprint_Loop_Lean_Lft_anm.bin");
	m_iSprintLeanRightAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Sprint_Loop_Lean_Rht_anm.bin");

	m_bAnimationIndicesCached = true;
}

void CPlayer_Locomotion_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;
	
	auto* pMoveIntent = player->GetMoveIntent();
	auto* pAnimator = player->GetAnimator();
	auto* pMotor = player->GetCharacterMotor();
	auto* pPlayerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!pMoveIntent || !pAnimator || !pMotor || !pPlayerStateMachine)
		return;

	// 내리막과 작은 단차에서는 CCT 접지가 잠깐 풀릴 수 있다. 즉시 FALL로
	// 전환하면 다시 지면을 만날 때 착지 애니메이션이 반복되므로 공중 상태가
	// 일정 시간 지속된 경우에만 실제 낙하로 판단한다.
	if (pMotor->IsGrounded())
	{
		m_fAirborneTime = 0.f;
	}
	else
	{
		m_fAirborneTime += fTimeDelta;
	}

	if (m_fAirborneTime >= m_fFallStateGraceTime &&
		pMotor->GetVelocity().y <= m_fFallStateVerticalSpeed)
	{
		m_fAirborneTime = 0.f;
		pPlayerStateMachine->RequestState(PLAYER_STATE::JUMP);
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
		m_bJogStarting = false;
		m_bJogStopping = false;
		m_iActiveMoveLoopAnimation = -1;
		player->SetRootMotionTranslationActive(false);
		player->SetMovementLocked(false);
		pMoveIntent->ClearMoveIntent();
		pMoveIntent->ClearFacingIntent();
		if (m_iIdleAnimation >= 0 &&
			pAnimator->GetPlayAnimIndex() !=
				static_cast<uint32_t>(m_iIdleAnimation))
		{
			pAnimator->Play_Anim(
				m_iIdleAnimation,
				true,
				0.15f);
		}
		return;
	}

	// 플레이어가 아직 회전하기 전의 Look을 기준으로 애니메이션용 각도를 구한다.
	m_fSignedMoveAngle = CalculateSignedAngle(*player, tMoveOutput.vMoveDirection);
	m_eMoveDirection = ResolveDirection(m_fSignedMoveAngle);

	// 정지 상태에서 처음 들어온 이동 입력은 곧바로 이동 루프로 연결한다.
	// 이 프레임의 큰 방향 차이는 달리기 중 방향 전환으로 취급하지 않는다.
	const _bool bStartedFromIdle = !m_bWasMoving;
	if (bStartedFromIdle)
	{
		m_bWasMoving = true;
		m_bJogStarting = false;
		player->SetMovementLocked(false);
		player->SetRootMotionTranslationActive(false);
	}

	// 일반 이동에서는 입력 방향으로 순간이동하듯 꺾지 않고
	// 연속적인 월드 방향을 유지한 채 일정 회전속도로 따라간다.
	if (player->IsSprintRequested())
	{
		pMoveIntent->SetFacingIntent(
			tMoveOutput.vMoveDirection,
			m_fSprintTurnSpeed);

		_vector vLook = XMVectorSetY(
			player->GetTransform().GetState(STATE::LOOK),
			0.f);
		_vector vRequestedDirection = XMVectorSetY(
			XMLoadFloat3(&tMoveOutput.vMoveDirection),
			0.f);
		if (XMVectorGetX(XMVector3LengthSq(vLook)) >
				std::numeric_limits<_float>::epsilon() &&
			XMVectorGetX(XMVector3LengthSq(vRequestedDirection)) >
				std::numeric_limits<_float>::epsilon())
		{
			const _vector vCurvedMoveDirection = XMVector3Normalize(
				XMVectorLerp(
					XMVector3Normalize(vLook),
					XMVector3Normalize(vRequestedDirection),
					m_fSprintMoveDirectionBlend));
			_float3 vMoveDirection{};
			XMStoreFloat3(&vMoveDirection, vCurvedMoveDirection);
			pMoveIntent->SetMoveIntent(
				vMoveDirection,
				player->GetCurrentMoveSpeed());
		}
	}
	else
	{
		pMoveIntent->SetFacingIntent(
			tMoveOutput.vMoveDirection,
			360.f);
	}

	int32_t iDesiredMoveAnimation =
		player->IsWalkRequested() && m_iWalkForwardAnimation >= 0
		? m_iWalkForwardAnimation : m_iJogForwardAnimation;
	if (player->IsSprintRequested())
	{
		const _float fLeanAngle = CalculateSignedAngle(
			*player,
			player->GetRawMoveDirection());

		const _bool bKeepRightLean =
			m_iActiveMoveLoopAnimation == m_iSprintLeanRightAnimation &&
			fLeanAngle >= 2.f;
		const _bool bKeepLeftLean =
			m_iActiveMoveLoopAnimation == m_iSprintLeanLeftAnimation &&
			fLeanAngle <= -2.f;

		if (bKeepRightLean ||
			fLeanAngle >= 7.f)
			iDesiredMoveAnimation = m_iSprintLeanRightAnimation;
		else if (bKeepLeftLean ||
			fLeanAngle <= -7.f)
			iDesiredMoveAnimation = m_iSprintLeanLeftAnimation;
		else
			iDesiredMoveAnimation = m_iSprintForwardAnimation;
	}

	if (iDesiredMoveAnimation >= 0 && iDesiredMoveAnimation != m_iActiveMoveLoopAnimation)
	{
		const _float fPreviousLoopRatio =
			m_iActiveMoveLoopAnimation >= 0
			? PlayerAnimationRatioGuard::Sanitize(
				pAnimator->GetPlayAnimRatio())
			: 0.f;
		pAnimator->Play_Anim(iDesiredMoveAnimation, true, 0.22f);

		auto* pModelInstance = player->GetModelInstance();
		if (pModelInstance && pModelInstance->GetModel())
		{
			const auto& animations =
				pModelInstance->GetModel()->GetAnimations();
			if (static_cast<size_t>(iDesiredMoveAnimation) <
				animations.size() &&
				animations[iDesiredMoveAnimation])
			{
				pAnimator->SetTrackPosition(
					animations[iDesiredMoveAnimation]->GetDuration() *
					fPreviousLoopRatio,
					true);
			}
		}

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
	player.SetMovementLocked(false);
	player.SetRootMotionTranslationActive(false);

	if (m_iJogStopForwardAnimation >= 0)
	{
		pAnimator->Play_Anim(
			m_iJogStopForwardAnimation,
			false,
			0.12f);
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

#if 0
void CPlayer_Locomotion_State::UpdateIdleTurnRotation(
	CPlayer& player,
	_float fAnimationRatio)
{
	const _float fRatio = std::clamp(fAnimationRatio, 0.f, 1.f);

	// 애니메이션 마지막 10%는 발 정리 구간으로 남기고,
	// 실제 회전은 SmoothStep으로 천천히 시작하고 천천히 끝낸다.
	const _float fRotationCompletionRatio = m_bJogTurning
		? m_fJogTurnRotationCompletionRatio
		: 0.9f;
	const _float fNormalizedTurnRatio =
		std::clamp(fRatio / fRotationCompletionRatio, 0.f, 1.f);
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
			player.SetCurrentMoveSpeed(m_fJogTurnEntrySpeed);
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
	m_fJogTurnEntrySpeed = 0.f;
	m_vJogTurnEntryDirection = {};
	m_iPendingIdleTurnAnimation = -1;
}
#endif

SPtr<CPlayer_Locomotion_State> CPlayer_Locomotion_State::Create()
{
	return ToSPtr(new CPlayer_Locomotion_State{});
}
