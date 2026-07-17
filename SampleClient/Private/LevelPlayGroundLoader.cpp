#include "pch.h"
#include "LevelPlayGroundLoader.h"
#include "GameInstance.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "TestGob.h"
#include "Terrain.h"
#include "LightObject.h"
#include "TestModel.h"
#include "Weapon.h"
NS_USING(Client)

std::future<bool> CLevelPlayGroundLoader::Load()
{
	// 메인 스레드 시작
	if (auto res = CGameInstance::Get().AddResource("LEVEL_PLAYGROUND", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
	{
		if (FAILED(res->Load()))
		{
			MSG_BOX("");
			//return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("LEVEL_PLAYGROUND", "Model_Resource_TombProtector",
		CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Protector/SK_Tomb_Protector.bin")))
	{
		E::CResModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
		if (FAILED(res->Load(pDesc)))
		{
			MSG_BOX("PLAY_GROUND Failed Model_Resource_TombProtector");
			//return false;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("LEVEL_PLAYGROUND", "Static_Axe_Model_Resource",
		CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/Tomb_Axe.fbx"))) 
	{
		E::CResStaticModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

		if (FAILED(res->Load(pDesc)))
		{
			MSG_BOX("PLAY_GROUND Failed Static_Axe_Model_Resource");
			//return false;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("LEVEL_PLAYGROUND", "Static_Mace_Model_Resource",
		CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/Tomb_Mace.fbx"))) {

		E::CResStaticModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

		if (FAILED(res->Load(pDesc)))
		{
			MSG_BOX("PLAY_GROUND Failed Static_Mace_Model_Resource");
			//return false;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("LEVEL_PLAYGROUND", "Static_Sword_Model_Resource",
		CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/Tomb_Sword.fbx"))) {

		E::CResStaticModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

		if (FAILED(res->Load(pDesc)))
		{
			MSG_BOX("PLAY_GROUND Failed Static_Sword_Model_Resource");
			//return false;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("LEVEL_PLAYGROUND", "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
	{
		if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
		{
			MSG_BOX("PLAY_GROUND Failed VIBUFFER_Terrain ");
			//return false;
		}
	}
	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_PLAY_GROUND", []()
		{
			CTerrain::DESC Terrain{};
			Terrain.tagLevelName = "LEVEL_PLAYGROUND";

			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_Terrain", CTerrain::Create(&Terrain))))
			{
				MSG_BOX("PLAY_GROUND Failed Prototype_GameObject_Terrain");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_Gobline", CTestGob::Create())))
			{
				MSG_BOX("PLAY_GROUND Failed Prototype_GameObject_Gobline");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_TestModel", CTestModel::Create())))
			{
				MSG_BOX("PLAY_GROUND Failed Prototype_GameObject_TestModel");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_Weapon", CWeapon::Create())))
			{
				MSG_BOX("PLAY_GROUND Failed Prototype_GameObject_Weapon");
				return false;
			}
			if (E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_Light", CLight::Create()))
			{
				MSG_BOX("PLAY_GROUND Failed Prototype_GameObject_Light");
				return false;
			}
			CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 1.f);

			return  true;
	});
			
}
HRESULT CLevelPlayGroundLoader::UnLoad()
{
	LOG_MEMORY("start");
	E::CGameInstance::Get().DelPrototype("LEVEL_PLAYGROUND");
	E::CGameInstance::Get().DelResource("LEVEL_PLAYGROUND");

	LOG_MEMORY("end");
	return S_OK;
}
