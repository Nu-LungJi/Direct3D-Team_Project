#include "pch.h"
#include "LevelBossCharlesRookwoodLoader.h"

#include "Level_Defines.h"

#include "GameInstance.h"
#include "BackGround.h"

#include "DebugPlayer.h"
#include "DebugPlayerThirdPersonCamera.h"

#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "Player_Weapon.h"
#include "Player_Magic_Bullet.h"
#include "BossTMB.h"
NS_USING(Client)

std::future<bool> CLevelBossCharlesRookwoodLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_BossCharlesRookwood", []()
		{
			// Map Load
			if (FAILED(E::CGameInstance::Get().LoadMapResources(MAP_PATH)))
			{
				return false;
			}


			// 디버그 플레이어 프로토타입 등록
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_DebugPlayer, CDebugPlayer::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_DebugPlayer");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_DebugPlayerThirdPersonCamera, CDebugPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_DebugPlayerThirdPersonCamera");
				return false;
			}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::BOSS_CHARLES_ROOKWOOD, "PLAYER_MODEL_RESROUCE", CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.5f, 0.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_MODEL_RESROUCE");
					return false;
				}
			}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::BOSS_CHARLES_ROOKWOOD, "PLAYER_WEAPON_RESROUCE", CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Wand.bin"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_WEAPON_RESROUCE");
					return false;
				}

			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Player, CPlayer::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_Player");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera, CPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerThirdPersonCamera");
				return false;
			}
			if (FAILED(MonsterLoad_InWorker()))
			{
				MSG_BOX("MonsterLoad Failed");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon, CPlayer_Weapon::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerWeapon");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerMagicBullet, CPlayer_Magic_Bullet::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerMagicBullet");
				return false;
			}
			return true;

			return  true;
		});
}

std::future<bool> CLevelBossCharlesRookwoodLoader::UnLoad()
{
	LOG_MEMORY("start");

	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();

	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_BossCharlesRookwood", []()
		{
			E::CGameInstance::Get().DelPrototype("MAPEDITOR");
			E::CGameInstance::Get().DelResource("MAPEDITOR");   E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);

			CGameInstance::Get().DelPrototype(LEVEL::BOSS_CHARLES_ROOKWOOD);

			CGameInstance::Get().DelResource("MODEL");
			CGameInstance::Get().DelResource(LEVEL::BOSS_CHARLES_ROOKWOOD);
			return true;
		});
}

HRESULT CLevelBossCharlesRookwoodLoader::MonsterLoad_InWorker()
{
	
	{
		//TombBos
		{
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::BOSS_CHARLES_ROOKWOOD, "Model_Resource_TombProtector",
				CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Protector/SK_Tomb_Protector.bin")))
			{
				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TombProtector");
					return E_FAIL;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossTMB, CBossTMB::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_BossTMB");
				return E_FAIL;
			}
		}
	
	}
	return S_OK;
}
