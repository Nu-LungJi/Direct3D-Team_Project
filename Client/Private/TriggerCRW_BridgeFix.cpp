#include "pch.h"
#include "TriggerCRW_BridgeFix.h"
#include "BridgeCRW.h"
#include "Player.h"
#include "ClientEvents.h"
#include "UIManager.h"
NS_USING(Client)

HRESULT CTriggerCRW_BridgeFix::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTriggerCRW_BridgeFix::Update(_float fTimeDelta)
{
	if (m_bSpawned && !m_bQuestAdvanced)
	{
		m_fFixElapsedTime += std::max(0.f, fTimeDelta);
		if (m_fFixElapsedTime >= 5.f)
		{
			m_bQuestAdvanced = true;
			GET_SINGLE(UIManager)->CreateOrChangeQuest(
				"퍼시벌 랙햄의 시험을 완료하기");
			CGameInstance::Get().EventPublish(
				FQuestUIGroupChanged{
					.Group = QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_BRIDGE,
					.Active = false,
					.UpdateMinimap = true,
					.UpdateQuestWidget = false
				});
			CGameInstance::Get().EventPublish(
				FQuestUIGroupChanged{
					.Group = QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_PORTAL,
					.Active = true,
					.QuestText = "퍼시벌 랙햄의 시험을 완료하기",
					.UpdateMinimap = true,
					.UpdateQuestWidget = false
				});
		}
	}

	if (m_hTiggeredPlayer)
	{
		if (CGameInstance::Get().KeyDown(DIK_4))
		{
			if (!m_bSpawned)
			{
				const auto* pLayer =
					CGameInstance::Get().GetGameObjectLayer("BridgeCRW");
				if (!pLayer || pLayer->empty())
					return;

				auto* pBridge = CGameInstance::Get()
					.GetGameObjectByHandleT<CBridgeCRW>(pLayer->front());
				if (!pBridge || !pBridge->RequestFix())
					return;

				m_bSpawned = true;
				m_fFixElapsedTime = 0.f;
			}
		}
	}
	
}

void CTriggerCRW_BridgeFix::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_BridgeFix] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
	auto pPlayer = Cast<CPlayer>(pObj);
	if (!pPlayer)
		return;

	m_hTiggeredPlayer = pPlayer->GetHandle();
}

void CTriggerCRW_BridgeFix::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_BridgeFix] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	m_hTiggeredPlayer = std::nullopt;
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
