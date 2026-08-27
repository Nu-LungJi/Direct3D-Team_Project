#include "pch.h"
#include "TriggerCRW_SpawnMonster1.h"
#include "MyMagicSquareStepController.h"

#include "TmbGurdian.h"
#include "Player.h"
#include "ClientEvents.h"
#include "UIManager.h"
NS_USING(Client)

HRESULT CTriggerCRW_SpawnMonster1::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

QUEST_UI_GROUP CTriggerCRW_SpawnMonster1::ResolveEncounterGroup() const
{
	// 동일 프로토타입을 두 전투 구역에서 재사용한다. 충돌 에디터에서
	// 두 번째 오브젝트 이름을 SpawnMonster2로 저장하면 위치와 무관하게
	// 2구역으로 판별되며, 월드 위치도 보조 판별값으로 사용한다.
	const auto objectTag = GetObjectTag();
	if (objectTag.find("SpawnMonster2") != std::string::npos ||
		GetTransform().GetPosition().z < 0.f)
	{
		return QUEST_UI_GROUP::ROOKWOOD_TRIAL_02;
	}
	return QUEST_UI_GROUP::ROOKWOOD_TRIAL_01;
}

void CTriggerCRW_SpawnMonster1::Update(E::_float fTimeDelta)
{
	CPhysXCollisionProxyObject::Update(fTimeDelta);

	{
		// 현재 전투 구역에서 소환한 경비병 두 마리 사망을 추적한다.
		constexpr size_t expectedMonsterCount = 2;
		if (!m_bSpawned || m_bTrialCompleted ||
			m_vSpawnedMonsterHandles.size() != expectedMonsterCount)
			return;

		const _bool allMonstersDefeated = std::all_of(
			m_vSpawnedMonsterHandles.begin(),
			m_vSpawnedMonsterHandles.end(),
			[](CHandle monsterHandle)
			{
				auto* monster = E::CGameInstance::Get().
					GetGameObjectByHandleT<CMonster>(monsterHandle);
				return !monster || monster->Get_CurrentHp() <= 0;
			});

		if (!allMonstersDefeated)
			return;

		m_bTrialCompleted = true;
		GET_SINGLE(UIManager)->CreateOrChangeQuest(
			"퍼시벌 랙햄의 시험을 완료하기");
		// 완료된 구역의 원과 미니맵/월드 마커를 정리한다.
		E::CGameInstance::Get().EventPublish(
			FQuestUIGroupChanged{
				.Group = m_eEncounterGroup,
				.Active = false,
				.UpdateQuestWidget = false
			});

		// 첫 구역 완료 직후에는 두 번째 전투로 향하는 이동 목표만 연다.
		// 이동 목표에 도착하면 UIController가 실제 2구역 그룹으로 전환한다.
		if (m_eEncounterGroup == QUEST_UI_GROUP::ROOKWOOD_TRIAL_01)
		{
			E::CGameInstance::Get().EventPublish(
				FQuestUIGroupChanged{
					.Group = QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_TRIAL_02,
					.Active = true,
					.QuestText = "퍼시벌 랙햄의 시험을 완료하기",
					.UpdateQuestWidget = false
				});
		}
	}

}

void CTriggerCRW_SpawnMonster1::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_SpawnMonster1] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	if (auto pPlayer = Cast<CPlayer>(pObj))
	{
		if (!m_bSpawned)
		{
			m_eEncounterGroup = ResolveEncounterGroup();
			m_bSpawned = true;
			GET_SINGLE(UIManager)->CreateOrChangeQuest(
				"경비병들을 쓰러트리기");
			E::CGameInstance::Get().EventPublish(
				FQuestUIGroupChanged{
					.Group = m_eEncounterGroup,
					.Active = true,
					.QuestText = "경비병들을 쓰러트리기",
					.UpdateMinimap = false,
					.UpdateQuestWidget = false
				});

			const _bool secondEncounter =
				m_eEncounterGroup == QUEST_UI_GROUP::ROOKWOOD_TRIAL_02;
			const _float3 spawnPositions[2] = {
				secondEncounter
					? _float3{ -259.f, -230.f, -109.236f }
					: _float3{ -185.f, -230.f, 147.f },
				secondEncounter
					? _float3{ -246.f, -230.f, -109.236f }
					: _float3{ -185.f, -230.f, 164.f }
			};

			{
				CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
				TmbGurdianDesc.sObjectTag = "TmbGurdian";
				TmbGurdianDesc.TargetHandle = pPlayer->GetHandle();
				TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::CHARLES_ROOKWOOD);
				TmbGurdianDesc.vPos = spawnPositions[0];
				TmbGurdianDesc.ReSourceTag = "Model_Resource_TMBGurdian";
				TmbGurdianDesc.resBeHaviorMajor = "BTJSON";
				TmbGurdianDesc.resBeHaviorMinor = "TOMB_BT_GURDIAN3";
				TmbGurdianDesc.WeaponProtoName = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_Mace);
				TmbGurdianDesc.WeaponResourceName = "Model_Resource_Mace";
				TmbGurdianDesc.MonType = MONSTER_TYPE::NORMAL;

				XMStoreFloat3(&TmbGurdianDesc.vScale, XMVectorSet(2.f, 2.f, 2.f, 1));
				auto NormalTmb = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, "02_TmbGurdian", &TmbGurdianDesc);

				if (!NormalTmb)
				{
					MSG_BOX("Create TmbGurdian Failed in Terrain");
					return ;
				}

				m_vSpawnedMonsterHandles.push_back(*NormalTmb);
			}
			{
				CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
				TmbGurdianDesc.sObjectTag = "TmbGurdian";
				TmbGurdianDesc.TargetHandle = pPlayer->GetHandle();
				TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::CHARLES_ROOKWOOD);
				TmbGurdianDesc.vPos = spawnPositions[1];
				TmbGurdianDesc.ReSourceTag = "Model_Resource_TMBGurdian";
				TmbGurdianDesc.resBeHaviorMajor = "BTJSON";
				TmbGurdianDesc.resBeHaviorMinor = "TOMB_BT_GURDIAN3";
				TmbGurdianDesc.WeaponProtoName = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_Axe);
				TmbGurdianDesc.WeaponResourceName = "Model_Resource_Axe";
				TmbGurdianDesc.MonType = MONSTER_TYPE::NORMAL;

				XMStoreFloat3(&TmbGurdianDesc.vScale, XMVectorSet(2.f, 2.f, 2.f, 1));
				auto NormalTmb = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, "02_TmbGurdian", &TmbGurdianDesc);

				if (!NormalTmb)
				{
					MSG_BOX("Create TmbGurdian Failed in Terrain");
					return ;
				}

				m_vSpawnedMonsterHandles.push_back(*NormalTmb);
			}
		}
	}
}

void CTriggerCRW_SpawnMonster1::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_SpawnMonster1] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTriggerCRW_SpawnMonster1> CTriggerCRW_SpawnMonster1::Create()
{
	auto instance = E::ToUPtr(new CTriggerCRW_SpawnMonster1{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTriggerCRW_SpawnMonster1::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTriggerCRW_SpawnMonster1{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
