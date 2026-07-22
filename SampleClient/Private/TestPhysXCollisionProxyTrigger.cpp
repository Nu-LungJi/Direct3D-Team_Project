#include "pch.h"
#include "TestPhysXCollisionProxyTrigger.h"

NS_USING(Client)

HRESULT CTestPhysXCollisionProxyTrigger::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTestPhysXCollisionProxyTrigger::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CollisionProxyTrigger] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTestPhysXCollisionProxyTrigger::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CollisionProxyTrigger] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTestPhysXCollisionProxyTrigger> CTestPhysXCollisionProxyTrigger::Create()
{
	auto instance = E::ToUPtr(new CTestPhysXCollisionProxyTrigger{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTestPhysXCollisionProxyTrigger::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTestPhysXCollisionProxyTrigger{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
