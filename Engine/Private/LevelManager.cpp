#include "pch.h"
#include "LevelManager.h"
#include "GameInstance.h"
#include "Level.h"

NS_USING(Engine)

CLevelManager::CLevelManager()
{
}

CLevelManager::~CLevelManager()
{
}

HRESULT CLevelManager::ChangeLevel(UPtr<CLevel> pNewLevel)
{
	assert(pNewLevel);
	m_pLevelBeforeLevelChange = std::move(pNewLevel);
	return S_OK;

	assert(pNewLevel);
	//if (pNewLevel == nullptr)
	//{
	//	return E_FAIL;
	//}

	if (m_pCurrentLevel != nullptr)
	{
		//CGameInstance::Get().ClearResource(m_iCurrentLevelIndex);
		m_pCurrentLevel.reset();
	}

	m_pCurrentLevel = std::move(pNewLevel);

	return S_OK;
}

void CLevelManager::Update(_float fTimeDelta)
{
	if (m_pCurrentLevel == nullptr)
	{
		return;
	}
	m_pCurrentLevel->Update(fTimeDelta);
}

HRESULT CLevelManager::Render()
{
	if (m_pCurrentLevel == nullptr)
	{
		return E_FAIL;
	}

	return m_pCurrentLevel->Render();
}

void CLevelManager::UpdateGUI()
{

	ImGui::Begin("CLevelManager");
	for (auto& [Name, Func] : m_LevelChangeFuncs)
	{
		if (ImGui::Button(Name.c_str()))
		{
			Func();
		}
	}
	ImGui::End();


	//m_pCurrentLevel->UpdateGUI();
}

void CLevelManager::FrameStart(_float fTimeDelta)
{
	if (m_pLevelBeforeLevelChange.get())
	{
		if (m_pCurrentLevel.get())
		{
			//m_pCurrentLevel->BeforeLevelChange();
			m_pCurrentLevel.reset();
		}

		m_pCurrentLevel = std::move(m_pLevelBeforeLevelChange);
		//m_pCurrentLevel->AfterLevelChange();
	}

	m_pCurrentLevel->FrameStart(fTimeDelta);
}

void CLevelManager::FrameEnd(_float fTimeDelta)
{
	m_pCurrentLevel->FrameEnd(fTimeDelta);
}

UPtr<CLevelManager> CLevelManager::Create()
{
	return UPtr<CLevelManager>(new CLevelManager{});
}

