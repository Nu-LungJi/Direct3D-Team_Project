#include "pch.h"
#include "TriggerCRW_ToBoss.h"
#include "BridgeCRW.h"
#include "Level_Defines.h"
#include "LevelBossCharlesRookwood.h"
#include "LevelLoading.h"
NS_USING(Client)

HRESULT CTriggerCRW_ToBoss::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTriggerCRW_ToBoss::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_ToBoss] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	if (!m_bSpawned)
	{
		CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext(),
			LEVEL::BOSS_CHARLES_ROOKWOOD));
		//CGameInstance::Get().ChangeLevel(CLevelBossCharlesRookwood::Create());

		m_bSpawned = true;
	}
}

void CTriggerCRW_ToBoss::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_ToBoss] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTriggerCRW_ToBoss> CTriggerCRW_ToBoss::Create()
{
	auto instance = E::ToUPtr(new CTriggerCRW_ToBoss{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTriggerCRW_ToBoss::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTriggerCRW_ToBoss{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
