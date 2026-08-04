#pragma once

#include "PhysXCollisionProxyObject.h"
#include "Timer.h"
NS_BEGIN(Client)

class CTriggerCRW_SpawnStep2 final : public E::CPhysXCollisionProxyObject
{
public:
	struct DELAYED_TASK
	{
		E::CTimer Timer;
		std::function<void()> Callback;
		CHandle hStep{};
	};
	std::vector<DELAYED_TASK> m_DelayedTasks;
public:
	DECLARE_DERIVED_TYPE(CTriggerCRW_SpawnStep2, E::CPhysXCollisionProxyObject)

private:
	CTriggerCRW_SpawnStep2() = default;
	CTriggerCRW_SpawnStep2(const CTriggerCRW_SpawnStep2&) = default;
	~CTriggerCRW_SpawnStep2() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void Update(_float fTimeDelta) override;
	void OnTriggerEnter(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;

public:
	static E::UPtr<CTriggerCRW_SpawnStep2> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	_bool m_bSpawned{ false };
};

NS_END
