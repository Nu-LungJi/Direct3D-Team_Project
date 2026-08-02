#include "pch.h"
#include "TriggerCRW_SpawnMonster1.h"
#include "MyMagicSquareStepController.h"

#include "TmbGurdian.h"
#include "Player.h"
NS_USING(Client)

HRESULT CTriggerCRW_SpawnMonster1::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
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
			m_bSpawned = true;

			{
				CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
				TmbGurdianDesc.sObjectTag = "TmbGurdian";
				TmbGurdianDesc.TargetHandle = pPlayer->GetHandle();
				TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::CHARLES_ROOKWOOD);
				TmbGurdianDesc.vPos = _float3(-185.f, -230.f, 147.f);
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
			}
			{
				CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
				TmbGurdianDesc.sObjectTag = "TmbGurdian";
				TmbGurdianDesc.TargetHandle = pPlayer->GetHandle();
				TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::CHARLES_ROOKWOOD);
				TmbGurdianDesc.vPos = _float3(-185.f, -230.f, 164.f);
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
