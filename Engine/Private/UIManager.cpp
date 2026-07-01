#include "pch.h"
#include "UIManager.h"
#include "GameInstance.h"

CUIManager::CUIManager()
{
}

CUIManager::~CUIManager()
{
}

void CUIManager::UpdateGUI()
{
	ImGui::Begin("CUIManager");



	ImGui::End();

}

void CUIManager::Load()
{
}

void CUIManager::Find_UiDesc()
{
}

UPtr<CUIManager> CUIManager::Create()
{
	return UPtr<CUIManager>(new CUIManager{});
}
