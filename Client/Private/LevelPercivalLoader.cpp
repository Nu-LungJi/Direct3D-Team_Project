#include "pch.h"
#include "LevelPercivalLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

NS_USING(Client)

std::future<bool> CLevelPercivalLoader::Load()
{
	// 메인 스레드 시작
	

	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_PERCIBAL", []()
		{
			// 워커 스레드
			
			// 워커 스레드 종료
			return  true;
		});
}

HRESULT CLevelPercivalLoader::UnLoad()
{
	LOG_MEMORY("start");
	
	LOG_MEMORY("end");
	return S_OK;
}
