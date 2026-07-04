#include "pch.h"
#include "PrefabManager.h"

CPrefabManager::CPrefabManager()
{
}

CPrefabManager::~CPrefabManager()
{
}

HRESULT CPrefabManager::Initialize()
{
	return S_OK;
}

UPtr<CPrefabManager> CPrefabManager::Create()
{
	auto pInstance = ToUPtr(new CPrefabManager{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}
