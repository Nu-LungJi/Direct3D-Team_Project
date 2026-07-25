#include "pch.h"
#include "LevelCharlesRookwoodLoader.h"
#include "GameInstance.h"

#include "Player.h"
#include "DebugPlayer.h"
#include "DebugPlayerThirdPersonCamera.h"
#include "PlayerThirdPersonCamera.h"
#include "Level_Defines.h"

#include "TriggerCRW_SpawnStep.h"
#include "TriggerCRW_StairStep.h"
#include "MyMagicSquareStep.h"
#include "MyMagicSquareStepController.h"

#include "TriggerCRW_SpawnStep2.h"

#include "TriggerCRW_SpawnStep3.h"

#include "TriggerCRW_SpawnStep4.h"

#include "TriggerCRW_DeSpawnStep.h"
#include "TriggerCRW_DeSpawnStep2.h"
#include "TriggerCRW_DeSpawnStep3.h"
#include "TriggerCRW_DeSpawnStep4.h"

#include "BridgeCRW.h"

#include "TriggerCRW_BridgeBring.h"
#include "TriggerCRW_BridgeFix.h"

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

			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("TEST", "Model_Resource",
				CResModel::Create("./Resources/SampleClient/Models/Skeleton/Bridge/SK_Bridge.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(90.f));
				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed SK_Bridge.bin");
					return false;
				}
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

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_StairStep, CTriggerCRW_StairStep::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_StairStep");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_SpawnStep2, CTriggerCRW_SpawnStep2::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_SpawnStep2");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_SpawnStep3, CTriggerCRW_SpawnStep3::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_SpawnStep3");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_SpawnStep4, CTriggerCRW_SpawnStep4::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_SpawnStep4");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_DeSpawnStep, CTriggerCRW_DeSpawnStep::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_DeSpawnStep");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_DeSpawnStep2, CTriggerCRW_DeSpawnStep2::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_DeSpawnStep2");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_DeSpawnStep3, CTriggerCRW_DeSpawnStep3::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_DeSpawnStep3");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_DeSpawnStep4, CTriggerCRW_DeSpawnStep4::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_DeSpawnStep4");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_BridgeBring, CTriggerCRW_BridgeBring::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_BridgeBring");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_BridgeFix, CTriggerCRW_BridgeFix::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_TriggerCRW_BridgeFix");
					return false;
				}
			}

			// 매직스텝
			{
				if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(
					LEVEL::CHARLES_ROOKWOOD, "Static_SquareStep_A_Resource",
					CResStaticModel::Create(
						"./Resources/SampleClient/Models/Static/Sanctum/SM_SanctumDun_SquareStep_A.bin")))
				{
					E::CResStaticModel::DESC Desc{};
					Desc.PreTransformMatrix =
						XMMatrixRotationX(XMConvertToRadians(0.f));
					if (FAILED(res->Load(Desc)))
					{
						MSG_BOX("LEVEL_CREATURE Failed Static_SquareStep_A_Resource");
						return false;
					}
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_MyMagicSquareStep, CMyMagicSquareStep::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_MyMagicSquareStep");
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(
					LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_MyMagicSquareStepController, CMyMagicSquareStepController::Create())))
				{
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_MyMagicSquareStepController");
					return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BridgeCRW, CBridgeCRW::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_BridgeCRW");
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
			
			// 트리거 프록시 제거
			{
				CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_SpawnStep);
				CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_StairStep);
				CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_SpawnStep2);
				CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_SpawnStep3);
				CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_SpawnStep4);

				CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_DeSpawnStep);
				CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_DeSpawnStep2);
				CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_DeSpawnStep3);
				CGameInstance::Get().DelPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_TriggerCRW_DeSpawnStep4);
			}

			CGameInstance::Get().DelResource("MODEL");

			return true;
		});
}
