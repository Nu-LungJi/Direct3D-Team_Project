#pragma once

#include "PhysXCollisionProxyObject.h"

NS_BEGIN(Client)

class CTriggerCRW_DeSpawnStep final : public E::CPhysXCollisionProxyObject
{
public:
	DECLARE_DERIVED_TYPE(CTriggerCRW_DeSpawnStep, E::CPhysXCollisionProxyObject)

private:
	CTriggerCRW_DeSpawnStep() = default;
	CTriggerCRW_DeSpawnStep(const CTriggerCRW_DeSpawnStep&) = default;
	~CTriggerCRW_DeSpawnStep() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void OnTriggerEnter(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;

public:
	static E::UPtr<CTriggerCRW_DeSpawnStep> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	_bool m_bSpawned{ false };
};

NS_END
