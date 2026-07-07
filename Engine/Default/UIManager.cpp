#include "pch.h"
#include "UIManager.h"

CUIManager::CUIManager()
{
}

CUIManager::~CUIManager()
{
}

HRESULT CUIManager::Initialize()
{
	return S_OK;
}

UPtr<CUIManager> CUIManager::Create()
{
	auto pInstance = ToUPtr(new CUIManager{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}
