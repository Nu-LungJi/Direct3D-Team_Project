#include "pch.h"
#include "Player_Locomotion_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"

NS_USING(Client)

namespace
{
	constexpr _float START_TURN_THRESHOLD = 22.5f;
	constexpr _float PIVOT_THRESHOLD = 157.5f;
	constexpr _float IDLE_TURN_THRESHOLD = 30.f;
	constexpr _float LOOP_BLEND_DURATION = 0.15f;
	constexpr _float TRANSITION_BLEND_DURATION = 0.08f;
}

CPlayer_Locomotion_State::CPlayer_Locomotion_State()
{
	for (auto& table : m_LoopAnimations)
		table.fill(INVALID_ANIMATION);

	for (auto& phaseTable : m_StartAnimations)
		for (auto& table : phaseTable)
			table.fill(INVALID_ANIMATION);
	for (auto& phaseTable : m_StopAnimations)
		for (auto& table : phaseTable)
			table.fill(INVALID_ANIMATION);
	for (auto& phaseTable : m_FreeTurnStartAnimations)
		for (auto& table : phaseTable)
			table.fill(INVALID_ANIMATION);

	for (auto& from : m_GaitTransitions)
		for (auto& to : from)
			to.fill(INVALID_ANIMATION);
	for (auto& turns : m_IdleTurns)
		turns.fill(INVALID_ANIMATION);
	for (auto& pivots : m_JogPivots)
		pivots.fill(INVALID_ANIMATION);
	for (auto& gait : m_FreeTurnStart180)
		for (auto& side : gait)
			side.fill(INVALID_ANIMATION);
}

void CPlayer_Locomotion_State::Enter(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine
		? CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(pStateMachine->GetOwnerHandle())
		: nullptr;
	if (!player)
		return;

	if (!m_bAnimationTableInitialized)
		InitializeAnimationTable(*player);

	m_eCurrentGait = GAIT::END;
	m_ePendingGait = GAIT::END;
	m_eTransition = TRANSITION::NONE;
	m_iTransientAnimation = INVALID_ANIMATION;
	m_eLastDirection = MOVE_DIRECTION::FRONT;

	if (auto* animator = player->GetAnimator(); animator && m_iIdleAnimation != INVALID_ANIMATION)
		animator->Play_Anim(m_iIdleAnimation, true, LOOP_BLEND_DURATION);
}

void CPlayer_Locomotion_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	(void)fTimeDelta;

	auto* player = pStateMachine
		? CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(pStateMachine->GetOwnerHandle())
		: nullptr;
	if (!player)
		return;

	auto* animator = player->GetAnimator();
	auto* moveIntent = player->GetMoveIntent();
	if (!animator || !moveIntent || !m_bAnimationTableInitialized)
		return;

	if (UpdateTransient(*player, *animator))
		return;

	const _bool bHasMoveInput = player->HasMoveInput();
	const _bool bHover = player->GetLocomotionMode() == CPlayer::LOCOMOTION_MODE::HOVER;
	const _float fMoveAngle = bHasMoveInput
		? CalculateSignedAngle(*player, player->GetDesiredMoveDirection())
		: 0.f;
	const MOVE_DIRECTION eInputDirection = ResolveDirection(fMoveAngle);

	if (bHasMoveInput)
	{
		static constexpr _string_view sDirectionNames[] =
		{
			"FRONT", "RIGHT_45", "RIGHT_90", "RIGHT_135",
			"BACKWARD", "LEFT_135", "LEFT_90", "LEFT_45"
		};
		const _vector vMove = XMLoadFloat3(&player->GetDesiredMoveDirection());
		const _vector vLook = XMVector3Normalize(XMVectorSetY(
			player->GetTransform().GetState(STATE::LOOK), 0.f));
		const _vector vRight = XMVector3Normalize(XMVectorSetY(
			player->GetTransform().GetState(STATE::RIGHT), 0.f));
		player->SetLocomotionAngleDebug(
			XMVectorGetX(XMVector3Dot(vMove, vLook)),
			XMVectorGetX(XMVector3Dot(vMove, vRight)),
			fMoveAngle,
			moveIntent->GetOutput().fMoveSpeed,
			sDirectionNames[ETOUI(eInputDirection)]);
	}
	else
	{
		player->ClearLocomotionAngleDebug();
	}

	if (!bHasMoveInput)
	{
		if (m_eCurrentGait != GAIT::END)
		{
			const FOOT_PHASE ePhase = ResolveFootPhase(animator->GetPlayAnimRatio());
			const MOVE_DIRECTION eStopDirection = bHover
				? m_eLastDirection
				: MOVE_DIRECTION::FRONT;
			const int32_t iStop = FindPhasedDirectionalAnimation(
				m_StopAnimations, ePhase, m_eCurrentGait, eStopDirection);

			if (PlayTransient(
				*animator, iStop, TRANSITION::STOP, GAIT::END,
				TRANSITION_BLEND_DURATION))
			{
				return;
			}

			m_eCurrentGait = GAIT::END;
		}

		if (bHover && TryStartIdleTurn(*player, *animator))
			return;

		if (m_iIdleAnimation != INVALID_ANIMATION)
			animator->Play_Anim(m_iIdleAnimation, true, LOOP_BLEND_DURATION);
		return;
	}

	GAIT eDesiredGait = ResolveDesiredGait(*player);
	if (eDesiredGait == GAIT::END)
		eDesiredGait = GAIT::JOG;

	if (m_eCurrentGait == GAIT::END)
	{
		// Sprint에는 별도 Start가 없으므로 Jog Start 후 Jog2Sprint를 거친다.
		const GAIT eStartGait = eDesiredGait == GAIT::SPRINT ? GAIT::JOG : eDesiredGait;
		const FOOT_PHASE eStartPhase = FOOT_PHASE::LEFT;
		int32_t iStart = INVALID_ANIMATION;

		if (!bHover && std::abs(fMoveAngle) >= START_TURN_THRESHOLD)
		{
			const TURN_SIDE eSide = fMoveAngle < 0.f ? TURN_SIDE::LEFT : TURN_SIDE::RIGHT;
			if (eInputDirection == MOVE_DIRECTION::BACKWARD)
			{
				iStart = m_FreeTurnStart180[ETOUI(eStartGait)][ETOUI(eSide)][ETOUI(eStartPhase)];
			}
			else
			{
				iStart = FindPhasedDirectionalAnimation(
					m_FreeTurnStartAnimations,
					eStartPhase,
					eStartGait,
					eInputDirection);
			}
		}

		if (iStart == INVALID_ANIMATION)
		{
			const MOVE_DIRECTION eStartDirection = bHover
				? eInputDirection
				: MOVE_DIRECTION::FRONT;
			iStart = FindPhasedDirectionalAnimation(
				m_StartAnimations,
				eStartPhase,
				eStartGait,
				eStartDirection);
		}

		m_eLastDirection = bHover ? eInputDirection : MOVE_DIRECTION::FRONT;
		if (PlayTransient(
			*animator, iStart, TRANSITION::START, eStartGait,
			TRANSITION_BLEND_DURATION))
		{
			return;
		}

		m_eCurrentGait = eStartGait;
		PlayLoop(*player, *animator, m_eCurrentGait, m_eLastDirection);
		return;
	}

	// Walk와 Sprint 사이의 직접 전환은 사용하지 않고 Jog를 경유한다.
	GAIT eNextGait = eDesiredGait;
	if ((m_eCurrentGait == GAIT::WALK && eDesiredGait == GAIT::SPRINT) ||
		(m_eCurrentGait == GAIT::SPRINT && eDesiredGait == GAIT::WALK))
	{
		eNextGait = GAIT::JOG;
	}

	if (m_eCurrentGait != eNextGait)
	{
		const FOOT_PHASE ePhase = ResolveFootPhase(animator->GetPlayAnimRatio());
		const int32_t iTransition =
			m_GaitTransitions[ETOUI(m_eCurrentGait)][ETOUI(eNextGait)][ETOUI(ePhase)];
		if (PlayTransient(
			*animator, iTransition, TRANSITION::GAIT_CHANGE, eNextGait,
			TRANSITION_BLEND_DURATION))
		{
			return;
		}

		m_eCurrentGait = eNextGait;
	}

	if (!bHover && m_eCurrentGait == GAIT::JOG && std::abs(fMoveAngle) >= PIVOT_THRESHOLD)
	{
		const TURN_SIDE eSide = fMoveAngle < 0.f ? TURN_SIDE::LEFT : TURN_SIDE::RIGHT;
		const FOOT_PHASE ePhase = ResolveFootPhase(animator->GetPlayAnimRatio());
		const int32_t iPivot = m_JogPivots[ETOUI(eSide)][ETOUI(ePhase)];
		if (PlayTransient(
			*animator, iPivot, TRANSITION::PIVOT, m_eCurrentGait,
			TRANSITION_BLEND_DURATION))
		{
			return;
		}
	}

	m_eLastDirection = bHover ? eInputDirection : MOVE_DIRECTION::FRONT;
	PlayLoop(*player, *animator, m_eCurrentGait, m_eLastDirection);
}

CPlayer_Locomotion_State::MOVE_DIRECTION
CPlayer_Locomotion_State::ResolveDirection(_float fSignedAngle) const
{
	if (fSignedAngle >= -22.5f && fSignedAngle < 22.5f)
		return MOVE_DIRECTION::FRONT;
	if (fSignedAngle >= 22.5f && fSignedAngle < 67.5f)
		return MOVE_DIRECTION::RIGHT_45;
	if (fSignedAngle >= 67.5f && fSignedAngle < 112.5f)
		return MOVE_DIRECTION::RIGHT_90;
	if (fSignedAngle >= 112.5f && fSignedAngle < 157.5f)
		return MOVE_DIRECTION::RIGHT_135;
	if (fSignedAngle >= 157.5f || fSignedAngle < -157.5f)
		return MOVE_DIRECTION::BACKWARD;
	if (fSignedAngle >= -157.5f && fSignedAngle < -112.5f)
		return MOVE_DIRECTION::LEFT_135;
	if (fSignedAngle >= -112.5f && fSignedAngle < -67.5f)
		return MOVE_DIRECTION::LEFT_90;
	return MOVE_DIRECTION::LEFT_45;
}

_float CPlayer_Locomotion_State::CalculateSignedAngle(
	const CPlayer& player,
	const _float3& vWorldDirection) const
{
	_vector vLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vRight = XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);
	_vector vTarget = XMVectorSetY(XMLoadFloat3(&vWorldDirection), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vTarget)) <= std::numeric_limits<_float>::epsilon())
		return 0.f;

	vLook = XMVector3Normalize(vLook);
	vRight = XMVector3Normalize(vRight);
	vTarget = XMVector3Normalize(vTarget);

	const _float fForward = XMVectorGetX(XMVector3Dot(vTarget, vLook));
	const _float fRight = XMVectorGetX(XMVector3Dot(vTarget, vRight));
	return XMConvertToDegrees(std::atan2(fRight, fForward));
}

CPlayer_Locomotion_State::FOOT_PHASE
CPlayer_Locomotion_State::ResolveFootPhase(_float fAnimationRatio) const
{
	// 이름 기반 1차 매핑. 실제 재생 후 LU/RU가 반대면 이 반환만 교체하면 된다.
	return fAnimationRatio < 0.5f ? FOOT_PHASE::LEFT : FOOT_PHASE::RIGHT;
}

CPlayer_Locomotion_State::GAIT
CPlayer_Locomotion_State::ResolveDesiredGait(const CPlayer& player) const
{
	switch (player.GetDesiredGait())
	{
	case CPlayer::LOCOMOTION_GAIT::WALK:   return GAIT::WALK;
	case CPlayer::LOCOMOTION_GAIT::JOG:    return GAIT::JOG;
	case CPlayer::LOCOMOTION_GAIT::SPRINT: return GAIT::SPRINT;
	default:                               return GAIT::END;
	}
}

int32_t CPlayer_Locomotion_State::FindDirectionalAnimation(
	const GAIT_DIRECTION_TABLE& table,
	GAIT eGait,
	MOVE_DIRECTION eDirection) const
{
	if (eGait == GAIT::END || eDirection == MOVE_DIRECTION::END)
		return INVALID_ANIMATION;

	int32_t iAnimation = table[ETOUI(eGait)][ETOUI(eDirection)];
	if (iAnimation == INVALID_ANIMATION && eDirection != MOVE_DIRECTION::FRONT)
		iAnimation = table[ETOUI(eGait)][ETOUI(MOVE_DIRECTION::FRONT)];
	return iAnimation;
}

int32_t CPlayer_Locomotion_State::FindPhasedDirectionalAnimation(
	const PHASE_GAIT_DIRECTION_TABLE& table,
	FOOT_PHASE ePhase,
	GAIT eGait,
	MOVE_DIRECTION eDirection) const
{
	if (ePhase == FOOT_PHASE::END || eGait == GAIT::END ||
		eDirection == MOVE_DIRECTION::END)
	{
		return INVALID_ANIMATION;
	}

	int32_t iAnimation = table[ETOUI(ePhase)][ETOUI(eGait)][ETOUI(eDirection)];
	if (iAnimation == INVALID_ANIMATION && eDirection != MOVE_DIRECTION::FRONT)
		iAnimation = table[ETOUI(ePhase)][ETOUI(eGait)][ETOUI(MOVE_DIRECTION::FRONT)];
	if (iAnimation == INVALID_ANIMATION)
	{
		const FOOT_PHASE eOther = ePhase == FOOT_PHASE::LEFT
			? FOOT_PHASE::RIGHT
			: FOOT_PHASE::LEFT;
		iAnimation = table[ETOUI(eOther)][ETOUI(eGait)][ETOUI(eDirection)];
		if (iAnimation == INVALID_ANIMATION && eDirection != MOVE_DIRECTION::FRONT)
			iAnimation = table[ETOUI(eOther)][ETOUI(eGait)][ETOUI(MOVE_DIRECTION::FRONT)];
	}
	return iAnimation;
}

_bool CPlayer_Locomotion_State::PlayTransient(
	CComAnimator& animator,
	int32_t iAnimationIndex,
	TRANSITION eTransition,
	GAIT ePendingGait,
	_float fBlendDuration)
{
	if (iAnimationIndex == INVALID_ANIMATION)
		return false;

	animator.Play_Anim(iAnimationIndex, false, fBlendDuration);
	m_eTransition = eTransition;
	m_ePendingGait = ePendingGait;
	m_iTransientAnimation = iAnimationIndex;
	return true;
}

void CPlayer_Locomotion_State::PlayLoop(
	CPlayer& player,
	CComAnimator& animator,
	GAIT eGait,
	MOVE_DIRECTION eDirection)
{
	if (eGait == GAIT::END)
	{
		if (m_iIdleAnimation != INVALID_ANIMATION)
			animator.Play_Anim(m_iIdleAnimation, true, LOOP_BLEND_DURATION);
		return;
	}

	if (eGait == GAIT::SPRINT)
		eDirection = MOVE_DIRECTION::FRONT;

	const int32_t iLoop = FindDirectionalAnimation(m_LoopAnimations, eGait, eDirection);
	if (iLoop != INVALID_ANIMATION)
		animator.Play_Anim(iLoop, true, LOOP_BLEND_DURATION);
	else if (m_iIdleAnimation != INVALID_ANIMATION)
		animator.Play_Anim(m_iIdleAnimation, true, LOOP_BLEND_DURATION);

	(void)player;
}

_bool CPlayer_Locomotion_State::UpdateTransient(
	CPlayer& player,
	CComAnimator& animator)
{
	if (m_eTransition == TRANSITION::NONE)
		return false;

	if (static_cast<int32_t>(animator.GetPlayAnimIndex()) != m_iTransientAnimation)
	{
		m_eTransition = TRANSITION::NONE;
		m_iTransientAnimation = INVALID_ANIMATION;
		return false;
	}

	if (!animator.GetFinish())
		return true;

	const TRANSITION eCompleted = m_eTransition;
	m_eTransition = TRANSITION::NONE;
	m_iTransientAnimation = INVALID_ANIMATION;

	if (eCompleted == TRANSITION::STOP || eCompleted == TRANSITION::IDLE_TURN)
	{
		m_eCurrentGait = GAIT::END;
		m_ePendingGait = GAIT::END;
	}
	else
	{
		m_eCurrentGait = m_ePendingGait;
		m_ePendingGait = GAIT::END;
	}

	(void)player;
	return false;
}

_bool CPlayer_Locomotion_State::TryStartIdleTurn(
	CPlayer& player,
	CComAnimator& animator)
{
	const _float fAngle = CalculateSignedAngle(player, player.GetCameraFacingDirection());
	const _float fAbsAngle = std::abs(fAngle);
	if (fAbsAngle < IDLE_TURN_THRESHOLD)
		return false;

	const TURN_SIDE eSide = fAngle < 0.f ? TURN_SIDE::LEFT : TURN_SIDE::RIGHT;
	uint32_t iTurnSize = 0;
	if (fAbsAngle >= 157.5f)
		iTurnSize = 3;
	else if (fAbsAngle >= 112.5f)
		iTurnSize = 2;
	else if (fAbsAngle >= 67.5f)
		iTurnSize = 1;

	return PlayTransient(
		animator,
		m_IdleTurns[ETOUI(eSide)][iTurnSize],
		TRANSITION::IDLE_TURN,
		GAIT::END,
		TRANSITION_BLEND_DURATION);
}

SPtr<CPlayer_Locomotion_State> CPlayer_Locomotion_State::Create()
{
	return ToSPtr(new CPlayer_Locomotion_State{});
}

void CPlayer_Locomotion_State::InitializeAnimationTable(CPlayer& player)
{
	auto find = [&player](_string_view sName)
	{
		return player.FindAnimationIndex(sName);
	};
	auto setLoop = [this, &find](GAIT eGait, MOVE_DIRECTION eDirection, _string_view sName)
	{
		m_LoopAnimations[ETOUI(eGait)][ETOUI(eDirection)] = find(sName);
	};
	auto setPhased = [&find](
		PHASE_GAIT_DIRECTION_TABLE& table,
		FOOT_PHASE ePhase,
		GAIT eGait,
		MOVE_DIRECTION eDirection,
		_string_view sName)
	{
		table[ETOUI(ePhase)][ETOUI(eGait)][ETOUI(eDirection)] = find(sName);
	};

	static constexpr _string_view sDirectionTokens[] =
	{
		"Fwd", "Rht_45", "Rht_90", "Rht_135",
		"Bwd", "Lft_135", "Lft_90", "Lft_45"
	};

	m_iIdleAnimation = find("AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_anm.bin");

	for (uint32_t i = 0; i < ETOUI(MOVE_DIRECTION::END); ++i)
	{
		const auto eDirection = static_cast<MOVE_DIRECTION>(i);
		const auto sToken = sDirectionTokens[i];
		const _string sTokenString{ sToken };

		setLoop(GAIT::JOG, eDirection,
			"AN_ProfessorSharp_MasterRig_Hu_BM_Jog_Loop_" + sTokenString + "_anm.bin");
		if (eDirection != MOVE_DIRECTION::LEFT_90)
		{
			setLoop(GAIT::WALK, eDirection,
				"AN_ProfessorSharp_MasterRig_Hu_BM_Walk_Loop_" + sTokenString + "_anm.bin");
		}

		for (uint32_t phase = 0; phase < ETOUI(FOOT_PHASE::END); ++phase)
		{
			const auto ePhase = static_cast<FOOT_PHASE>(phase);
			const _string_view sFoot = ePhase == FOOT_PHASE::LEFT ? "LF" : "RF";
			const _string sFootString{ sFoot };

			setPhased(m_StartAnimations, ePhase, GAIT::WALK, eDirection,
				"AN_ProfessorSharp_MasterRig_Hu_BM_" + sFootString + "_Walk_Start_" + sTokenString + "_anm.bin");
			setPhased(m_StopAnimations, ePhase, GAIT::WALK, eDirection,
				"AN_ProfessorSharp_MasterRig_Hu_BM_" + sFootString + "_Walk_Stop_" + sTokenString + "_anm.bin");
			setPhased(m_StartAnimations, ePhase, GAIT::JOG, eDirection,
				"AN_ProfessorSharp_MasterRig_Hu_BM_" + sFootString + "_Jog_Start_" + sTokenString + "_anm.bin");
			setPhased(m_StopAnimations, ePhase, GAIT::JOG, eDirection,
				"AN_ProfessorSharp_MasterRig_Hu_BM_" + sFootString + "_Jog_Stop_" + sTokenString + "_anm.bin");
		}
	}

	// Walk_Loop_Lft_90은 원본 목록에 없어서 Lft_112를 90도 슬롯의 대체로 사용한다.
	setLoop(GAIT::WALK, MOVE_DIRECTION::LEFT_90,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Walk_Loop_Lft_112_anm.bin");
	setLoop(GAIT::SPRINT, MOVE_DIRECTION::FRONT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Sprint_Loop_Fwd_anm.bin");
	setPhased(m_StopAnimations, FOOT_PHASE::LEFT, GAIT::SPRINT, MOVE_DIRECTION::FRONT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_LF_Sprint_Stop_Fwd_anm.bin");
	setPhased(m_StopAnimations, FOOT_PHASE::RIGHT, GAIT::SPRINT, MOVE_DIRECTION::FRONT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_RF_Sprint_Stop_Fwd_anm.bin");

	for (GAIT eGait : { GAIT::WALK, GAIT::JOG })
	{
		const _string_view sGait = eGait == GAIT::WALK ? "Walk" : "Jog";
		for (uint32_t phase = 0; phase < ETOUI(FOOT_PHASE::END); ++phase)
		{
			const auto ePhase = static_cast<FOOT_PHASE>(phase);
			const _string_view sFoot = ePhase == FOOT_PHASE::LEFT ? "LF" : "RF";
			for (MOVE_DIRECTION eDirection :
				{ MOVE_DIRECTION::RIGHT_45, MOVE_DIRECTION::RIGHT_90, MOVE_DIRECTION::RIGHT_135,
				  MOVE_DIRECTION::LEFT_135, MOVE_DIRECTION::LEFT_90, MOVE_DIRECTION::LEFT_45 })
			{
				const auto sToken = sDirectionTokens[ETOUI(eDirection)];
				setPhased(m_FreeTurnStartAnimations, ePhase, eGait, eDirection,
					"AN_ProfessorSharp_MasterRig_Hu_BM_" + _string{ sFoot } + "_" +
					_string{ sGait } + "_Turn_Start_" + _string{ sToken } + "_anm.bin");
			}

			for (uint32_t side = 0; side < ETOUI(TURN_SIDE::END); ++side)
			{
				const auto eSide = static_cast<TURN_SIDE>(side);
				const _string_view sSide = eSide == TURN_SIDE::LEFT ? "Lft" : "Rht";
				m_FreeTurnStart180[ETOUI(eGait)][side][phase] = find(
					"AN_ProfessorSharp_MasterRig_Hu_BM_" + _string{ sFoot } + "_" +
					_string{ sGait } + "_Turn_Start_" + _string{ sSide } + "_180_anm.bin");
			}
		}
	}

	static constexpr uint32_t sTurnAngles[] = { 45, 90, 135, 180 };
	for (uint32_t side = 0; side < ETOUI(TURN_SIDE::END); ++side)
	{
		const _string_view sSide = side == ETOUI(TURN_SIDE::LEFT) ? "Lft" : "Rht";
		for (uint32_t angle = 0; angle < std::size(sTurnAngles); ++angle)
		{
			m_IdleTurns[side][angle] = find(
				"AN_ProfessorSharp_MasterRig_Hu_BM_LF_Idle_Turn_" + _string{ sSide } + "_" +
				std::to_string(sTurnAngles[angle]) + "_anm.bin");
		}
		for (uint32_t phase = 0; phase < ETOUI(FOOT_PHASE::END); ++phase)
		{
			const _string_view sPhase = phase == ETOUI(FOOT_PHASE::LEFT) ? "LU" : "RU";
			m_JogPivots[side][phase] = find(
				"AN_ProfessorSharp_MasterRig_Hu_BM_Jog_Pivot_" + _string{ sSide } +
				"_180_" + _string{ sPhase } + "_anm.bin");
		}
	}

	auto setGaitTransition = [this, &find](
		GAIT eFrom, GAIT eTo, FOOT_PHASE ePhase, _string_view sName)
	{
		m_GaitTransitions[ETOUI(eFrom)][ETOUI(eTo)][ETOUI(ePhase)] = find(sName);
	};
	setGaitTransition(GAIT::WALK, GAIT::JOG, FOOT_PHASE::LEFT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Walk2Jog_LU_anm.bin");
	setGaitTransition(GAIT::WALK, GAIT::JOG, FOOT_PHASE::RIGHT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Walk2Jog_RU_anm.bin");
	setGaitTransition(GAIT::JOG, GAIT::WALK, FOOT_PHASE::LEFT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Jog2Walk_LU_anm.bin");
	setGaitTransition(GAIT::JOG, GAIT::WALK, FOOT_PHASE::RIGHT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Jog2Walk_RU_anm.bin");
	setGaitTransition(GAIT::JOG, GAIT::SPRINT, FOOT_PHASE::LEFT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Jog2Sprint_LU_anm.bin");
	setGaitTransition(GAIT::JOG, GAIT::SPRINT, FOOT_PHASE::RIGHT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Jog2Sprint_RU_anm.bin");
	setGaitTransition(GAIT::SPRINT, GAIT::JOG, FOOT_PHASE::LEFT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Sprint2Jog_LU_anm.bin");
	setGaitTransition(GAIT::SPRINT, GAIT::JOG, FOOT_PHASE::RIGHT,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Sprint2Jog_RU_anm.bin");

	m_bAnimationTableInitialized = m_iIdleAnimation != INVALID_ANIMATION;
}
