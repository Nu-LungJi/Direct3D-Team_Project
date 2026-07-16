#include "pch.h"
#include "LevelAnimatorLoader.h"
#include "TestModel.h"
#include "Test_StaticModel.h"
#include "GameInstance.h"
#include "BackGround.h"

NS_USING(Client)

std::future<bool> CLevelAnimatorLoader::Load()
{

	//if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("TEST", "Model_Resource",
	//	CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Protector/SK_Tomb_Protector.bin"))) {

	//	E::CResModel::DESC pDesc{};
	//	pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));

	//	res->Load(pDesc);
	//}


	if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("TEST", "Model_Resource",
		CResModel::Create("./Resources/SampleClient/Models/Skeleton/Dragon/SK_Dragon.bin"))) {

		E::CResModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));

		if (FAILED(res->Load(pDesc))){
			MSG_BOX("FAILED DRAGON");
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("TEST", "Static_Model_Resource",
		CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/HorseStatue.fbx"))) {

		E::CResStaticModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

		res->Load(pDesc);
	}


	// 메인 스레드 종료
	 return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_LEVEL", []()
		{

			 if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TEST", "Prototype_GameObject_TestModel", CTestModel::Create())))
			 {
				 return false;
			 }

			 if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TEST", "Prototype_GameObject_TestStaticModel", CTest_StaticModel::Create())))
			 {
				 return false;
			 }
			 if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TEST", "Prototype_GameObject_TestStaticModel2", CTest_StaticModel::Create())))
			 {
				 return false;
			 }


			// 워커 스레드
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TEST", "Prototype_GameObject_BackGround", CBackGround::Create())))
			{
				return false;
			}
			// 워커 스레드 종료
			return  true;
	});
}

HRESULT CLevelAnimatorLoader::UnLoad()
{
	LOG_MEMORY("start");

	CGameInstance::Get().DelResource("TEST", "Model_Resource");
	CGameInstance::Get().DelResource("TEST", "Static_Model_Resource");

	E::CGameInstance::Get().DelPrototype("LEVEL_TEST");
	E::CGameInstance::Get().DelResource("LEVEL_TEST");

	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelResource("LIGHT");
	LOG_MEMORY("end");
	return S_OK;
}
