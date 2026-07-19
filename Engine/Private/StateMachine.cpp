#include "pch.h"
#include "StateMachine.h"

NS_USING(Engine)

CStateMachine::CStateMachine(const CStateMachine& rhs)
	: CComponent{ rhs }
{
}

HRESULT CStateMachine::Initialize(void* pArg)
{
	if (FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	if (!GetGameObject())
		return E_FAIL;

	m_hOwner = GetGameObject()->GetHandle();
	m_States.clear();
	m_pCurrentState.reset();

	return S_OK;
}

void CStateMachine::AddState(uint32_t iStateID, SPtr<CState> pState)
{
	if (pState)
		m_States[iStateID] = std::move(pState);
}

void CStateMachine::ChangeState(uint32_t iNextStateID)
{
	const auto iter = m_States.find(iNextStateID);
	if (iter == m_States.end() || !iter->second || m_pCurrentState == iter->second)
		return;

	if (m_pCurrentState)
		m_pCurrentState->Exit(this);

	m_pCurrentState = iter->second;
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

UPtr<CStateMachine> CStateMachine::Create()
{
	auto pInstance = ToUPtr(new CStateMachine{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to create CStateMachine");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CStateMachine::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CStateMachine{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to clone CStateMachine");
		return nullptr;
	}

	return pInstance;
}

void CStateMachine::Free()
{
	m_pCurrentState.reset();
	m_States.clear();
	m_hOwner = {};
	CComponent::Free();
}
