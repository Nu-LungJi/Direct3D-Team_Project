#pragma once

#include "PhysXCollisionProxyObject.h"

NS_BEGIN(Client)

class CTriggerCRW_SpawnMonster1 final : public E::CPhysXCollisionProxyObject
{
public:
	DECLARE_DERIVED_TYPE(CTriggerCRW_SpawnMonster1, E::CPhysXCollisionProxyObject)

private:
	CTriggerCRW_SpawnMonster1() = default;
	CTriggerCRW_SpawnMonster1(const CTriggerCRW_SpawnMonster1&) = default;
	~CTriggerCRW_SpawnMonster1() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void OnTriggerEnter(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;

public:
	static E::UPtr<CTriggerCRW_SpawnMonster1> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	_bool m_bSpawned{ false };
};

NS_END
