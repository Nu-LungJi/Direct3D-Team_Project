#pragma once
#include "Component.h"
#include "GameObject.h"

NS_BEGIN(Engine)

class CStateMachine;

class ENGINE_DLL CState : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CState, CEngineBase)

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


class ENGINE_DLL CStateMachine : public CComponent
{
public:
	struct DESC : public CComponent::DESC
	{
	};

public:
	DECLARE_DERIVED_TYPE(CStateMachine, CComponent)

protected:
	CStateMachine() = default;
	CStateMachine(const CStateMachine& rhs);
	~CStateMachine() override = default;

protected:
	HRESULT Initialize(void* pArg) override;

public:
	void AddState(uint32_t iStateID, SPtr<CState> pState);
	void ChangeState(uint32_t iNextStateID);

	void PriorityUpdate(_float fTimeDelta);
	void Update(_float fTimeDelta);
	void LateUpdate(_float fTimeDelta);

	template <typename T>
	T* GetOwner() const
	{
		// 상태 전환은 GameObject가 매니저에 등록되기 전인 Initialize 단계에서도
		// 발생할 수 있다. Handle 기반 조회는 그 시점에 nullptr을 반환하므로,
		// 컴포넌트가 이미 보유한 직접 소유자 포인터를 사용한다.
		return Cast<T>(GetGameObject());
	}

	const CHandle& GetOwnerHandle() const { return m_hOwner; }

	static UPtr<CStateMachine> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	std::unordered_map<uint32_t, SPtr<CState>> m_States{};
	SPtr<CState> m_pCurrentState{};
	CHandle m_hOwner{};

protected:
	void Free() override;
};

NS_END
