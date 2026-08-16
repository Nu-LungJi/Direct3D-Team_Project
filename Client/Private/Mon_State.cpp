#include "pch.h"
#include "Mon_State.h"

NS_USING(Client)
CMon_State::CMon_State()
{
}

CMon_State::CMon_State(const CMon_State& rhs) : CStateMachine{ rhs }
{
}

CMon_State::~CMon_State()
{
}

HRESULT CMon_State::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

_bool CMon_State::Add_State(MON_STATE eState, SPtr<CState> pState)
{
	if (eState == MON_STATE::NONE || eState == MON_STATE::END || nullptr == pState || IsRegistered(eState))
		return false;

	__super::AddState(ETOUI(eState), std::move(pState));
	m_RegisteredState.insert(ETOUI(eState));

	return true;
}

_bool CMon_State::Initialize_State(MON_STATE eState)
{
	//초기상태 지정
	if (m_eCurState != MON_STATE::NONE || !IsRegistered(eState))
		return false;

	__super::ChangeState(ETOUI(eState));
	m_eCurState = eState;

	return true;
}

_bool CMon_State::Request_State(MON_STATE eState)
{
	if (!IsRegistered(eState) || m_eCurState == eState)
		return false;

	m_eRequestState = eState;

	return true;
}
void CMon_State::ApplyStateRequest()
{
	//상태가 없으면 아무것도 하지 않음
	if (m_eRequestState == MON_STATE::NONE)
		return;

	//예약된 상태를 이동하고 기존 예약 칸을 비움
	MON_STATE eNextState = m_eRequestState;
	m_eRequestState = MON_STATE::NONE;

	if (!IsRegistered(eNextState) || m_eCurState == eNextState)
		return;

	__super::ChangeState(ETOUI(eNextState));
	m_eCurState = eNextState;
}
void CMon_State::PriorityUpdate(_float fTimeDelta)
{
	ApplyStateRequest();
	__super::PriorityUpdate(fTimeDelta);
}
_bool CMon_State::IsRegistered(MON_STATE eState)
{
	if (eState == MON_STATE::NONE || eState == MON_STATE::END)
		return false;

	return m_RegisteredState.contains(ETOUI(eState));
}


UPtr<CMon_State> CMon_State::Create()
{
	auto pInstance = ToUPtr(new CMon_State{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to create CMon_State");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CMon_State::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CMon_State{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to clone CMon_State");
		return nullptr;
	}

	return pInstance;
}
