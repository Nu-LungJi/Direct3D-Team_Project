#include "pch.h"
#include "LevelCharlesRookwoodLoader.h"
#include "GameInstance.h"

#include "Player.h"
#include "DebugPlayer.h"
#include "DebugPlayerThirdPersonCamera.h"
#include "PlayerThirdPersonCamera.h"
#include "Level_Defines.h"

#include "TriggerCRW_SpawnStep.h"

#include "TmbGurdian.h"
#include "Weapon.h"
NS_USING(Client)

std::future<bool> CLevelCharlesRookwoodLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_CharlesRookwood", []()
		{
			// Map Load
			if (FAILED(E::CGameInstance::Get().LoadMapResources(MAP_PATH)))
			{
				return false;
			}
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("MODEL", "PLAYER_MODEL_RESROUCE",CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f , 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.5f, 0.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_Player");
					return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Player, CPlayer::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_Player");
				return false;
			}

			// 디버그 플레이어 프로토타입 등록
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_DebugPlayer, CDebugPlayer::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_DebugPlayer");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_DebugPlayerThirdPersonCamera, CDebugPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_DebugPlayerThirdPersonCamera");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera, CPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerThirdPersonCamera");
				return false;
			}

			// 찰리스록우드맵 관련 트리거 프로토 타입
			{
				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_SpawnStep, CTriggerCRW_SpawnStep::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_SpawnStep");
					return false;
				}
			}

			if (FAILED(MonsterLoad_InWorker()))
			{
				MSG_BOX("Create Failed Monster in CharlesRookwood");
				return false;
			}
			return true;
		});
}

std::future<bool> CLevelCharlesRookwoodLoader::UnLoad()
{
	LOG_MEMORY("start");

	// 메인스레드 MAP해제
	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();
	
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_CharlesRookwood", []()
		{
			// 워커스레드 MAP 해제
			E::CGameInstance::Get().DelPrototype("MAPEDITOR");
			E::CGameInstance::Get().DelResource("MAPEDITOR");   E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);

			CGameInstance::Get().DelPrototype(LEVEL::CHARLES_ROOKWOOD);
			CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_SpawnStep);

			CGameInstance::Get().DelResource("MODEL");

			CGameInstance::Get().DelResource(LEVEL::CHARLES_ROOKWOOD);
			return true;
		});
}
HRESULT CLevelCharlesRookwoodLoader::MonsterLoad_InWorker()
{
	//TombGurDian
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::CHARLES_ROOKWOOD, "Model_Resource_TMBGurdian",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Grunt/SK_Tomb_Grunt.bin")))
		{
			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TMBGurdian");
				//return false;
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, CTmbGurdian::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TMBGurdian");
			return false;
		}

	}

	//Weapon
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Axe,
			CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Tomb_Axe.bin"))) {

			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Static_Axe_Model_Resource");
				return false;
			}
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Sword,
			CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Tomb_Sword.bin"))) {

			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Static_Sword_Model_Resource");
				return false;
			}
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Mace,
			CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Tomb_Mace.bin"))) {

			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Static_Mace_Model_Resource");
				return false;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Axe, CWeapon::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Axe");
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Sword, CWeapon::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Sword");
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Mace, CWeapon::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Mace");
			return false;
		}
	}
}
