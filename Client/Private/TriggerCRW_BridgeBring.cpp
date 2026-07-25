#include "pch.h"
#include "TriggerCRW_BridgeBring.h"
#include "MyMagicSquareStepController.h"
NS_USING(Client)

HRESULT CTriggerCRW_BridgeBring::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTriggerCRW_BridgeBring::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_BridgeBring] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	if (!m_bSpawned)
	{

	}
}

void CTriggerCRW_BridgeBring::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_BridgeBring] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTriggerCRW_BridgeBring> CTriggerCRW_BridgeBring::Create()
{
	auto instance = E::ToUPtr(new CTriggerCRW_BridgeBring{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTriggerCRW_BridgeBring::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTriggerCRW_BridgeBring{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
