#include "pch.h"
#include "Player_StateMachine.h"

NS_USING(Client)

CPlayer_StateMachine::CPlayer_StateMachine(const CPlayer_StateMachine& rhs)
	: CStateMachine{ rhs }
{
}

HRESULT CPlayer_StateMachine::Initialize(void* pArg)
{
	if (FAILED(CStateMachine::Initialize(pArg)))
		return E_FAIL;

	m_RegisteredStateIDs.clear();
	m_eCurrentState = PLAYER_STATE::NONE;
	m_eRequestedState = PLAYER_STATE::NONE;

	return S_OK;
}

_bool CPlayer_StateMachine::AddPlayerState(PLAYER_STATE eState, SPtr<CState> pState)
{
	if (eState == PLAYER_STATE::NONE || eState == PLAYER_STATE::END || !pState)
		return false;

	AddState(ETOUI(eState), std::move(pState));
	m_RegisteredStateIDs.insert(ETOUI(eState));
	return true;
}

_bool CPlayer_StateMachine::SetInitialState(PLAYER_STATE eState)
{
	if (m_eCurrentState != PLAYER_STATE::NONE || !IsRegistered(eState))
		return false;

	ChangeState(ETOUI(eState));
	m_eCurrentState = eState;
	return true;
}

_bool CPlayer_StateMachine::RequestState(PLAYER_STATE eState)
{
	if (!IsRegistered(eState) || !CanTransition(m_eCurrentState, eState))
		return false;

	if (m_eRequestedState != PLAYER_STATE::NONE &&
		GetTransitionPriority(eState) < GetTransitionPriority(m_eRequestedState))
	{
		return false;
	}

	m_eRequestedState = eState;
	return true;
}

void CPlayer_StateMachine::ApplyStateRequest()
{
	if (m_eRequestedState == PLAYER_STATE::NONE)
		return;

	const PLAYER_STATE eNextState = m_eRequestedState;
	m_eRequestedState = PLAYER_STATE::NONE;

	if (!CanTransition(m_eCurrentState, eNextState))
		return;

	ChangeState(ETOUI(eNextState));
	m_eCurrentState = eNextState;
}

void CPlayer_StateMachine::PriorityUpdate(_float fTimeDelta)
{
	ApplyStateRequest();
	__super::PriorityUpdate(fTimeDelta);
}

UPtr<CPlayer_StateMachine> CPlayer_StateMachine::Create()
{
	auto pInstance = ToUPtr(new CPlayer_StateMachine{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to create CPlayer_StateMachine");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CPlayer_StateMachine::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CPlayer_StateMachine{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to clone CPlayer_StateMachine");
		return nullptr;
	}

	return pInstance;
}

_bool CPlayer_StateMachine::IsRegistered(PLAYER_STATE eState) const
{
	return eState != PLAYER_STATE::NONE && eState != PLAYER_STATE::END && m_RegisteredStateIDs.contains(ETOUI(eState));
}

_bool CPlayer_StateMachine::IsInSkillState() const
{
	return IsSkillState(m_eCurrentState);
}

_bool CPlayer_StateMachine::IsSkillState(PLAYER_STATE eState)
{
	return eState > PLAYER_STATE::SKILL_BEGIN &&
		eState < PLAYER_STATE::SKILL_END;
}

_bool CPlayer_StateMachine::CanTransition(PLAYER_STATE eCurrent, PLAYER_STATE eNext) const
{
	if (eCurrent == eNext || eCurrent == PLAYER_STATE::DEAD)
		return false;

	if (IsSkillState(eCurrent))
	{
		return eNext == PLAYER_STATE::LOCOMOTION ||
			eNext == PLAYER_STATE::HIT ||
			eNext == PLAYER_STATE::DEAD;
	}

	return true;
}

uint32_t CPlayer_StateMachine::GetTransitionPriority(PLAYER_STATE eState) const
{
	switch (eState)
	{
	case PLAYER_STATE::DEAD:   return 100;
	case PLAYER_STATE::HIT:    return 80;
	case PLAYER_STATE::ROLL:   return 60;
	case PLAYER_STATE::JUMP:   return 50;
	case PLAYER_STATE::DASH_SKILL: return 45;
	case PLAYER_STATE::ACIENTATTACK_SKILL: return 45;
	case PLAYER_STATE::ACCIO_SKILL: return 45;
	case PLAYER_STATE::DEPULSO_SKILL: return 45;
	case PLAYER_STATE::DESCENDO_SKILL: return 45;
	case PLAYER_STATE::BOMBARDA_SKILL: return 45;
	case PLAYER_STATE::CONFRINGO_SKILL: return 45;
	case PLAYER_STATE::AVADA_KEDAVRA_SKILL: return 45;
	case PLAYER_STATE::LUMOS_SKILL: return 45;
	case PLAYER_STATE::REVELIO_SKILL: return 45;
	case PLAYER_STATE::REPAIRO_SKILL: return 45;
	case PLAYER_STATE::ATTACK: return 40;
	default:                   return 0;
	}
}

void CPlayer_StateMachine::Free()
{
	m_RegisteredStateIDs.clear();
	CStateMachine::Free();
}
