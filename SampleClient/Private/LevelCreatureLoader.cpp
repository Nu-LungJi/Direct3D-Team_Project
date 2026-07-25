#include "pch.h"
#include "LevelCreatureLoader.h"

#include "GameInstance.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "TestGob.h"
#include "Terrain.h"
#include "LightObject.h"
#include "TestModel.h"
#include "Weapon.h"
#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "Player_StateMachine.h"
#include "TestPlayerCreatureEditor.h"
#include "TestPlayer3CameraCreatureEditor.h"
#include "Test3DSound.h"
#include "MapCollisionProxyObject.h"
#include "TestPhysXCollisionProxyTrigger.h"
#include "TestDynamic.h"

#include "MyMagicSquareStepController.h"
#include "MyMagicSquareStep.h"
NS_USING(Client)

std::future<bool> CLevelCreatureLoader::Load()
{

	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_LEVEL_CREATURE", []()
		{
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE", "Prototype_GameObject_Test3DSound", CTest3DSound::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TestModel");
				return false;
			}

			// 메인 스레드 시작


			if (auto res = CGameInstance::Get().AddResource("LEVEL_CREATURE", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
			{
				if (FAILED(res->Load()))
				{
					MSG_BOX("");
					//return E_FAIL;
				}
			}


			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(
				"LEVEL_CREATURE",
				"Model_Resource_Player",
				CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin")))
			{
				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.5f, 0.f);
				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Model_Resource_Player");
					//return false;
				}
			}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("LEVEL_CREATURE", "Model_Resource_TombProtector",
				CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Protector/SK_Tomb_Protector.bin")))
			{
				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TombProtector");
					//return false;
				}
			}
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("LEVEL_CREATURE", "Model_Resource_TombNormalProtector",
				CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Grunt/SK_Tomb_Grunt.bin")))
			{
				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TombNormalProtector");
					//return false;
				}
			}
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("LEVEL_CREATURE", "Model_Resource_Dragon",
				CResModel::Create("./Resources/SampleClient/Models/Skeleton/Dragon/SK_Dragon.bin")))
			{
				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Model_Resource_Dragon");
					//return false;
				}
			}
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("LEVEL_CREATURE", "Static_Axe_Model_Resource",
				CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/Tomb_Axe.fbx")))
			{
				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Static_Axe_Model_Resource");
					//return false;
				}
			}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("LEVEL_CREATURE", "Static_Mace_Model_Resource",
				CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/Tomb_Mace.fbx"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Static_Mace_Model_Resource");
					//return false;
				}
			}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("LEVEL_CREATURE", "Static_Sword_Model_Resource",
				CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/Tomb_Sword.fbx"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Static_Sword_Model_Resource");
					//return false;
				}
			}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("LEVEL_CREATURE", "Static_OilBarrel_Resource",
				CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_oil_barrel_0001.bin"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(300.f, 300.f, 300.f);

				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Static_OilBarrel_Resource");
					//return false;
				}
			}
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(
				"LEVEL_CREATURE", "Static_SquareStep_A_Resource",
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
			if (auto res = CGameInstance::Get().AddResource("LEVEL_CREATURE", "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
			{
				if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
				{
					MSG_BOX("LEVEL_CREATURE Failed VIBUFFER_Terrain ");
					//return false;
				}
			}

			CTerrain::DESC Terrain{};
			Terrain.tagLevelName = "LEVEL_CREATURE";
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE", "Prototype_GameObject_Terrain", CTerrain::Create(&Terrain))))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Terrain");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				"LEVEL_CREATURE", "Prototype_GameObject_MapCollisionProxy", CMapCollisionProxyObject::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_MapCollisionProxy");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				E::PX_COLLISION_PROXY_PROTOTYPE_GROUP,
				"Prototype_GameObject_TestPhysXCollisionProxyTrigger",
				CTestPhysXCollisionProxyTrigger::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TestPhysXCollisionProxyTrigger");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE", "Prototype_GameObject_Gobline", CTestGob::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Gobline");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE", "Prototype_GameObject_TestModel", CTestModel::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TestModel");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				"LEVEL_CREATURE", "Prototype_GameObject_TestDynamic", CTestDynamic::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TestDynamic");
				return false;
			}
			
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("LEVEL_PLAYGROUND", "Static_Wand_Model_Resource",
				CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Wand.bin")))
			{
				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("PLAY_GROUND Failed Static_Wand_Model_Resource");
					//return false;
				}
			}
			
			

			//MyMagicSquareStep
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				"LEVEL_CREATURE",
				"Prototype_GameObject_MyMagicSquareStep",
				CMyMagicSquareStep::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_MyMagicSquareStep");
				return false;
			}
			//CMyMagicSquareStepController
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				"LEVEL_CREATURE",
				"Prototype_GameObject_MyMagicSquareStepController",
				CMyMagicSquareStepController::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_MyMagicSquareStepController");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE", "Prototype_GameObject_Weapon", CWeapon::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Weapon");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				"PLAYER_STATEMACHINE",
				"Prototype_Component_Player_StateMachine",
				CPlayer_StateMachine::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_Component_Player_StateMachine");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_Wand", CWeapon::Create())))
			{
				MSG_BOX("PLAY_GROUND Failed Prototype_GameObject_Wand");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE", "Prototype_GameObject_Player", CPlayer::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Player");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE","Prototype_GameObject_PlayerThirdPersonCamera",
				CPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_PlayerThirdPersonCamera");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				"LEVEL_CREATURE", "Prototype_GameObject_TestPlayerCreatureEditor",
				CTestPlayerCreatureEditor::Create())))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				"LEVEL_CREATURE", "Prototype_GameObject_TestPlayer3CameraCreatureEditor",
				CTestPlayer3CameraCreatureEditor::Create())))
			{
				return false;
			}
			if (E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE", "Prototype_GameObject_Light", CLight::Create()))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Light");
				return false;
			}
			CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 1.f);

			return  true;
		});

}

std::future<bool> CLevelCreatureLoader::UnLoad()
{
	LOG_MEMORY("start");
	CGameInstance::Get().GetSoundManager()->ClearResources();

	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_LEVEL_CREATURE", []()
		{
			E::CGameInstance::Get().DelPrototype("LEVEL_CREATURE");
			E::CGameInstance::Get().DelPrototype(
				E::PX_COLLISION_PROXY_PROTOTYPE_GROUP,
				"Prototype_GameObject_TestPhysXCollisionProxyTrigger");
			E::CGameInstance::Get().DelResource("LEVEL_CREATURE");

			return  true;
		});
}
