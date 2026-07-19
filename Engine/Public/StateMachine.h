// StateMachine.h
#pragma once
#include "Engine_Defines.h"
#include "GameInstance.h"

NS_BEGIN(Engine)

class CStateMachine;

class ENGINE_DLL CState : public CEngineBase
{
protected:
	CState() = default;
	~CState() override = default;

public:
	virtual void Enter(CStateMachine* pStateMachine) {}
	virtual void Exit(CStateMachine* pStateMachine) {}

	virtual void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) {}
	virtual void Update(CStateMachine* pStateMachine, _float fTimeDelta) {}
	virtual void LateUpdate(CStateMachine* pStateMachine, _float fTimeDelta) {}
};

class ENGINE_DLL CStateMachine : public CEngineBase
{
protected:
	CStateMachine() = default;
	~CStateMachine() override = default;

public:
	HRESULT Initialize(const CHandle& hOwner);

	void AddState(uint32_t iStateID, SPtr<CState> pState);
	void ChangeState(uint32_t iNextStateID);

	void PriorityUpdate(_float fTimeDelta);
	void Update(_float fTimeDelta);
	void LateUpdate(_float fTimeDelta);

	template <typename T>
	T* GetOwner() const
	{
		return CGameInstance::Get().GetGameObjectByHandleT<T>(m_hOwner);
	}

	const CHandle& GetOwnerHandle() const
	{
		return m_hOwner;
	}

public:
	static SPtr<CStateMachine> Create(const CHandle& hOwner);

private:
	std::unordered_map<uint32_t, SPtr<CState>> m_States;
	SPtr<CState> m_pCurrentState;
	CHandle m_hOwner{};
};

NS_END
