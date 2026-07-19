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
#include "TestPlayerCreatureEditor.h"
#include "TestPlayer3CameraCreatureEditor.h"
NS_USING(Client)

std::future<bool> CLevelCreatureLoader::Load()
{

	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_LEVEL_CREATURE", []()
		{
			// 메인 스레드 시작
			if (auto res = CGameInstance::Get().AddResource("LEVEL_CREATURE", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
			{
				if (FAILED(res->Load()))
				{
					MSG_BOX("");
					//return E_FAIL;
				}
			}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("LEVEL_CREATURE", "Model_Resource_Player",
				CResModel::Create("./Resources/SampleClient/Models/Skeleton/Test/SK_Test.bin")))
			{
				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
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

			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE", "Prototype_GameObject_Weapon", CWeapon::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Weapon");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CREATURE", "Prototype_GameObject_Player", CPlayer::Create())))
			{
				MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Player");
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

	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_LEVEL_CREATURE", []()
		{
			E::CGameInstance::Get().DelPrototype("LEVEL_CREATURE");
			E::CGameInstance::Get().DelResource("LEVEL_CREATURE");

			return  true;
		});
}
