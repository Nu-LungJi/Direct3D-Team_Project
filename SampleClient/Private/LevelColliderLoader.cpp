#include "pch.h"
#include "LevelColliderLoader.h"

#include "GameInstance.h"

#include "TestCollider.h"

NS_USING(Client)

std::future<bool> CLevelColliderLoader::Load()
{
	// 메인 스레드 시작
	
	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_COLLIDER", []()
		{
			if (!(CGameInstance::Get().AddResourceT<CResLuaScript>("SampleClient_Lua", "Hi", CResLuaScript::CreateAndLoad("./LuaFiles/ClientTest/asdf.lua"))))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_COLLIDER", "Prototype_GameObject_TestCollider", CTestCollider::Create())))
			{
				return false;
			}
			
			return  true;
		});
}

std::future<bool> CLevelColliderLoader::UnLoad()
{
	LOG_MEMORY("start");
	E::CGameInstance::Get().DelPrototype("LEVEL_COLLIDER");
	E::CGameInstance::Get().DelResource	("SampleClient_Lua");

	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelResource("LIGHT");
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_COLLIDER", []()
		{
			return true;
		});
}
