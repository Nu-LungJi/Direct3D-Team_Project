#include "pch.h"
#include "TriggerCRW_DeSpawnStep2.h"
#include "MyMagicSquareStepController.h"
#include "Player.h"
#include "TmbGurdian.h"
NS_USING(Client)

HRESULT CTriggerCRW_DeSpawnStep2::Initialize(void* pArg)
{
	return CPhysXCollisionProxyObject::Initialize(pArg);
}

void CTriggerCRW_DeSpawnStep2::OnTriggerEnter(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_DeSpawnStep2] Enter : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");

	if (auto pPlayer = Cast<CPlayer>(pObj))
	{
		if (!m_bSpawned)
		{
			m_bSpawned = true;

			auto pvec = CGameInstance::Get().GetGameObjectLayer("22_MyMagicSquareStepController");
			if (!(pvec || pvec->empty()))
			{
				return;
			}

			if (auto pController = CGameInstance::Get().GetGameObjectByHandleT<CMyMagicSquareStepController>(pvec->front()))
			{
				const StringID GroupID{ "MagicSquareGrid2" };
				pController->DeleteGroup(GroupID);
				//CMyMagicSquareStepController::RISE_PATTERN_DESC RiseDesc{};
				////RiseDesc.fStartTargetY = -227.f;
				//RiseDesc.fStartTargetY = -214.f;
				//RiseDesc.fEndTargetY = -214.f;
				//RiseDesc.fMoveSpeed = 15.f;
				//RiseDesc.fBounceHeight = 0.3f;
				//RiseDesc.fBounceSettleSpeed = 1.f;
				//RiseDesc.fLineInterval = 0.05f;
				//RiseDesc.fStepInterval = 0.02f;
				//RiseDesc.fStepTimingCurve = 0.55f;
				//RiseDesc.fStepTimingJitter = 1.01f;
				//RiseDesc.eFillMode =
				//	CMyMagicSquareStepController::
				//	RISE_FILL_MODE::Z;
				//RiseDesc.eHeightAxis =
				//	CMyMagicSquareStepController::
				//	FILL_AXIS::Z;
				//RiseDesc.eDirection =
				//	CMyMagicSquareStepController::
				//	FILL_DIRECTION::REVERSE;
				//if (!pController->StartRisePattern(GroupID, RiseDesc))
				//{
				//	return;
				//}
			}



			{
				CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
				TmbGurdianDesc.sObjectTag = "TmbGurdian";
				TmbGurdianDesc.TargetHandle = pPlayer->GetHandle();
				TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::CHARLES_ROOKWOOD);
				TmbGurdianDesc.vPos = _float3(-244.f, -230.3f, -121.f);
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
					return;
				}
			}
			{
				CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
				TmbGurdianDesc.sObjectTag = "TmbGurdian";
				TmbGurdianDesc.TargetHandle = pPlayer->GetHandle();
				TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::CHARLES_ROOKWOOD);
				TmbGurdianDesc.vPos = _float3(-258.f, -230.3f, -121.f);
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
					return;
				}
			}
		}
	}
	
}

void CTriggerCRW_DeSpawnStep2::OnTriggerExit(
	E::CGameObject* pObj, const E::PX_ON_TRIGGER_DATA& info)
{
	DEBUG_LOG_STR(std::string("[PX][CTriggerCRW_DeSpawnStep2] Exit : ") +
		(pObj ? std::string{ pObj->GetObjectTag() } : "null") + "\n");
}

E::UPtr<CTriggerCRW_DeSpawnStep2> CTriggerCRW_DeSpawnStep2::Create()
{
	auto instance = E::ToUPtr(new CTriggerCRW_DeSpawnStep2{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

E::UPtr<E::CPrototype> CTriggerCRW_DeSpawnStep2::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CTriggerCRW_DeSpawnStep2{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
