#include "pch.h"
#include "LevelLightMapLoader.h"

#include "GameInstance.h"
#include "BackGround.h"
#include "ResTerrainVIBuffer.h"
#include "Terrain.h"
#include "LightObject.h"
#include "TestModel.h"

NS_USING(Client)

std::future<bool> CLevelLightMapLoader::Load()
{
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
	{
		if (FAILED(res->Load()))
		{
			MSG_BOX("");
			//return E_FAIL;
		}
	}
	if (FAILED(E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_LightObject", CLightObject::Create())))
	{
		int a = 0;
		//return false;
	}
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_LIGHTMAP", []()
		{
			if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_BUFFER", "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
			{
				if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
				{
					//MSG_BOX("");
					return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_Terrain", CTerrain::Create())))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_TestModel", CTestModel::Create())))
			{
				return false;
			}

			return  true;
		});
}

HRESULT CLevelLightMapLoader::UnLoad()
{
	LOG_MEMORY("start");
	CGameInstance::Get().Clear_DynamicLightList();

	E::CGameInstance::Get().DelPrototype("LIGHT");
	E::CGameInstance::Get().DelResource("SAMPLE_CLIENT_TEX");
	E::CGameInstance::Get().DelResource("SAMPLE_CLIENT_BUFFER");
	E::CGameInstance::Get().DelResource("LOBJ", "Model_Resource");

	LOG_MEMORY("end");

	return S_OK;
}
