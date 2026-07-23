#pragma once

#include "PhysXCollisionProxyObject.h"

NS_BEGIN(Client)

class CTestPhysXCollisionProxyTrigger final : public E::CPhysXCollisionProxyObject
{
public:
	DECLARE_DERIVED_TYPE(CTestPhysXCollisionProxyTrigger, E::CPhysXCollisionProxyObject)

private:
	CTestPhysXCollisionProxyTrigger() = default;
	CTestPhysXCollisionProxyTrigger(const CTestPhysXCollisionProxyTrigger&) = default;
	~CTestPhysXCollisionProxyTrigger() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void OnTriggerEnter(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info) override;

public:
	static E::UPtr<CTestPhysXCollisionProxyTrigger> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
