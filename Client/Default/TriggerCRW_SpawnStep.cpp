#include "pch.h"
#include "TriggerCRW_SpawnStep.h"

NS_USING(Client)

HRESULT CTriggerCRW_SpawnStep::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTriggerCRW_SpawnStep::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_SpawnStep] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

void CTriggerCRW_SpawnStep::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_SpawnStep] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTriggerCRW_SpawnStep> CTriggerCRW_SpawnStep::Create()
{
	auto instance = E::ToUPtr(new CTriggerCRW_SpawnStep{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTriggerCRW_SpawnStep::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTriggerCRW_SpawnStep{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
