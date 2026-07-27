#pragma once
#include "PhysXCollisionProxyObject.h"

NS_BEGIN(Client)

class CTriggerCRW_StairStep final : public E::CPhysXCollisionProxyObject
{
public:
	DECLARE_DERIVED_TYPE(CTriggerCRW_StairStep, E::CPhysXCollisionProxyObject)

private:
	CTriggerCRW_StairStep() = default;
	CTriggerCRW_StairStep(const CTriggerCRW_StairStep&) = default;
	~CTriggerCRW_StairStep() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void OnTriggerEnter(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;

public:
	static E::UPtr<CTriggerCRW_StairStep> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	_bool m_bSpawned{ false };
};

NS_END
