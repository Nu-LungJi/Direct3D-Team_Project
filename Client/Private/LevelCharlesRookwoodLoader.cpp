#include "pch.h"
#include "LevelCharlesRookwoodLoader.h"
#include "GameInstance.h"

#include "Player.h"
#include "DebugPlayer.h"
#include "DebugPlayerThirdPersonCamera.h"
#include "PlayerThirdPersonCamera.h"
#include "Level_Defines.h"
#include "Player_Weapon.h"
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
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::CHARLES_ROOKWOOD, "PLAYER_MODEL_RESROUCE",CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f , 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.5f, 0.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_MODEL_RESROUCE");
					return false;
				}
			}
	/*		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::CHARLES_ROOKWOOD, "PLAYER_WEAPON_RESROUCE", CResModel::Create("./Resources/SampleClient/Models/Skeleton/Wand/SM_Wand.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_WEAPON_RESROUCE");
					return false;
				}
			}*/
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::CHARLES_ROOKWOOD, "PLAYER_WEAPON_RESROUCE", CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Wand.bin"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_WEAPON_RESROUCE");
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

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon, CPlayer_Weapon::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerWeapon");
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

			CGameInstance::Get().DelResource("MODEL");

			return true;
		});
}
