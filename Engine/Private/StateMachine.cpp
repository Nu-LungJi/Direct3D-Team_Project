// StateMachine.cpp
#include "pch.h"
#include "StateMachine.h"

NS_USING(Engine)

HRESULT CStateMachine::Initialize(const CHandle& hOwner)
{
	m_hOwner = hOwner;
	return S_OK;
}

void CStateMachine::AddState(uint32_t iStateID, SPtr<CState> pState)
{
	if (!pState)
		return;

	m_States[iStateID] = std::move(pState);
}

void CStateMachine::ChangeState(uint32_t iNextStateID)
{
	const auto iter = m_States.find(iNextStateID);

	if (iter == m_States.end() || !iter->second)
		return;

	// 현재 상태를 Exit()하는 동안 상태 테이블이 바뀌어도 다음 상태가 유지되도록 복사한다.
	SPtr<CState> pNextState = iter->second;

	if (m_pCurrentState == pNextState)
		return;

	if (m_pCurrentState)
		m_pCurrentState->Exit(this);

	m_pCurrentState = pNextState;
	m_pCurrentState->Enter(this);
}

void CStateMachine::PriorityUpdate(_float fTimeDelta)
{
	if (m_pCurrentState)
		m_pCurrentState->PriorityUpdate(this, fTimeDelta);
}

void CStateMachine::Update(_float fTimeDelta)
{
	if (m_pCurrentState)
		m_pCurrentState->Update(this, fTimeDelta);
}

void CStateMachine::LateUpdate(_float fTimeDelta)
{
	if (m_pCurrentState)
		m_pCurrentState->LateUpdate(this, fTimeDelta);
}

SPtr<CStateMachine> CStateMachine::Create(const CHandle& hOwner)
{
	auto pInstance = ToSPtr(new CStateMachine{});

	if (FAILED(pInstance->Initialize(hOwner)))
	{
		MSG_BOX("Failed to Created : CStateMachine");
		return nullptr;
	}

	return pInstance;
}
