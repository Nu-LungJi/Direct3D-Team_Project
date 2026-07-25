#include "pch.h"
#include "TriggerCRW_BridgeFix.h"
#include "MyMagicSquareStepController.h"
NS_USING(Client)

HRESULT CTriggerCRW_BridgeFix::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTriggerCRW_BridgeFix::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_SpawnStep] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	if (!m_bSpawned)
	{
		
	}
}

void CTriggerCRW_BridgeFix::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_BridgeFix] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTriggerCRW_BridgeFix> CTriggerCRW_BridgeFix::Create()
{
	auto instance = E::ToUPtr(new CTriggerCRW_BridgeFix{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTriggerCRW_BridgeFix::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTriggerCRW_BridgeFix{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
