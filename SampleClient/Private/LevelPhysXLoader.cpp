#include "pch.h"
#include "LevelPhysXLoader.h"

#include "GameInstance.h"
#include "Resources.h"
#include "TestCollider.h"
#include "LevelPhysX.h"
#include "TestPhysX.h"
#include "TestPhysXTerrain.h"
#include "TestPhysXBox.h"
#include "TestPhysXBall.h"
#include "TestPhysXCapsule.h"
#include "TestPhysXTrigger.h"

#include "Client_Resources.h"

#include "Terrain.h"

#include "TestCharacter.h"
#include "TestThirdPersonCamera.h"
#include "TestMonster.h"

NS_USING(Client)

std::future<bool> CLevelPhysXLoader::Load()
{
	// 메인 스레드 시작
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_TEX_PX", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
	{
		if (FAILED(res->Load()))
		{
			MSG_BOX("");
			//return E_FAIL;
		}
	}
	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_PhysX", []()
		{
			// 워커 스레드

			if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_BUFFER_PX", "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
			{
				if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
				{
					//MSG_BOX("");
					return false;
				}
			}

			//if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND_PX", "Prototype_GameObject_Terrain", CTerrain::Create())))
			//{
			//	return false;
			//}
			if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PX", "Prototype_GameObject_TestPhysX", CTestPhysX::Create())))
			{
				return false;
			}
			//TestPhysXTerrain
			if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PX", "Prototype_GameObject_TestPhysXTerrain", CTestPhysXTerrain::Create())))
			{
				return false;
			}

			//TestPhysXBox
			if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PX", "Prototype_GameObject_TestPhysXBox", CTestPhysXBox::Create())))
			{
				return false;
			}

			//TestPhysXBall
			if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PX", "Prototype_GameObject_TestPhysXBall", CTestPhysXBall::Create())))
			{
				return false;
			}

			//TestPhysXCapsule
			if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PX", "Prototype_GameObject_TestPhysXCapsule", CTestPhysXCapsule::Create())))
			{
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PX", "Prototype_GameObject_TestPhysXTrigger", CTestPhysXTrigger::Create())))
			{
				return false;
			}

			//TestCharacter
			if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PX", "Prototype_GameObject_TestCharacter", CTestCharacter::Create())))
			{
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PX", "Prototype_GameObject_TestThirdPersonCamera", CTestThirdPersonCamera::Create())))
			{
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PX", "Prototype_GameObject_TestMonster", CTestMonster::Create())))
			{
				return false;
			}
			// 워커 스레드 종료
			return  true;
		});
}
std::future<bool> CLevelPhysXLoader::UnLoad()
{
	LOG_MEMORY("start");

	E::CGameInstance::Get().DelResource("SAMPLE_CLIENT_TEX_PX");
	E::CGameInstance::Get().DelResource("SAMPLE_CLIENT_BUFFER_PX");
	E::CGameInstance::Get().DelPrototype("SAMPLE_CLIENT_PX");
	
	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelResource("LIGHT");

	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_PX", []()
		{
			return true;
		});
}
