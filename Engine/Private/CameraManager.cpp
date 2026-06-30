#include "pch.h"

#include "CameraManager.h"
#include "GameInstance.h"
#include "CameraObject.h"
NS_USING(Engine)

CCameraManager::CCameraManager()
{
}

CCameraManager::~CCameraManager()
{
}

void CCameraManager::UpdateGUI()
{
	ImGui::Begin("CCameraManager");
	std::string activeCamera{ "ACTIVE: " };
	if (m_ActiveCamera.has_value())
	{
		if (auto c = GetCamera(m_ActiveCamera->first))
		{
			activeCamera += m_ActiveCamera->first.GetDbgStr();
		}
	}
	ImGui::Text(activeCamera.c_str());
	if (ImGui::TreeNode("RegisteredCamera"))
	{
		for (const auto& camHandle : m_Cameras)
		{
			ImGui::PushID(camHandle.first.GetDbgStr());

			if (ImGui::TreeNode(camHandle.first.GetDbgStr()))
			{
				auto pObj = CGameInstance::Get().GetGameObjectByHandle(camHandle.second);

				if (pObj)
				{
					pObj->UpdateGUI();
				}

				ImGui::TreePop();
			}
			ImGui::SameLine();
			if (ImGui::Button("Active"))
			{
				SetActiveCamera(camHandle.first);
			}

			ImGui::PopID();
		}

		ImGui::TreePop();
	}



	//std::string activeGameCamera{"GAME: "};
	//if (m_ActiveGameCamera.has_value())
	//{
	//	if (auto c = GetGameCamera(m_ActiveGameCamera->first))
	//	{
	//		activeGameCamera += m_ActiveGameCamera->first.GetDbgStr();
	//	}
	//}
	//ImGui::Text(activeGameCamera.c_str());

	//std::string activeUICamera{ "UI: " };
	//if (m_ActiveUICamera.has_value())
	//{
	//	if (auto c = GetUICamera(m_ActiveUICamera->first))
	//	{
	//		activeUICamera += m_ActiveUICamera->first.GetDbgStr();
	//	}
	//}
	//ImGui::Text(activeUICamera.c_str());

	//if (ImGui::TreeNode("RegisteredGameCamera"))
	//{
	//	for (const auto& camHandle : m_GameCameras)
	//	{
	//		ImGui::PushID(camHandle.first.GetDbgStr());

	//		if (ImGui::TreeNode(camHandle.first.GetDbgStr()))
	//		{
	//			auto pObj = CGameInstance::Get().GetGameObjectByHandle(camHandle.second);

	//			if (pObj)
	//			{
	//				pObj->UpdateGUI();
	//			}

	//			ImGui::TreePop();
	//		}
	//		ImGui::SameLine();
	//		if (ImGui::Button("Active"))
	//		{
	//			SetActiveGameCamera(camHandle.first);
	//		}

	//		ImGui::PopID();
	//	}

	//	ImGui::TreePop();
	//}

	//if (ImGui::TreeNode("RegisteredUICamera"))
	//{
	//	for (const auto& camHandle : m_UICameras)
	//	{
	//		ImGui::PushID(camHandle.first.GetDbgStr());

	//		if (ImGui::TreeNode(camHandle.first.GetDbgStr()))
	//		{
	//			auto pObj = CGameInstance::Get().GetGameObjectByHandle(camHandle.second);

	//			if (pObj)
	//			{
	//				pObj->UpdateGUI();
	//			}

	//			ImGui::TreePop();
	//		}
	//		ImGui::SameLine();
	//		if (ImGui::Button("Active"))
	//		{
	//			SetActiveUICamera(camHandle.first);
	//		}

	//		ImGui::PopID();
	//	}

	//	ImGui::TreePop();
	//}



	ImGui::End();
}


CCameraObject* CCameraManager::GetActiveCamera() const
{
	if (!m_ActiveCamera.has_value())
	{
		return nullptr;
	}

	auto pObj = CGameInstance::Get().GetGameObjectByHandle(m_ActiveCamera->second);
	if (!pObj)
	{
		return nullptr;
	}

	if (!pObj->IsA(CCameraObject::StaticType))
	{
		return nullptr;
	}

	return static_cast<CCameraObject*>(pObj);
}

CCameraObject* CCameraManager::GetActiveCamera(const StringID& CameraID) const
{
	auto tmp = GetActiveCamera();
	if (tmp != GetCamera(CameraID))
	{
		return nullptr;
	}
	return tmp;
}

HRESULT CCameraManager::SetActiveCamera(const StringID& CameraID)
{
	auto iter = m_Cameras.find(CameraID);
	if (iter == m_Cameras.end())
	{
		return E_FAIL;
	}
	auto pObj = CGameInstance::Get().GetGameObjectByHandle(iter->second);
	if (!pObj)
	{
		return E_FAIL;
	}

	if (!pObj->IsA(CCameraObject::StaticType))
	{
		return E_FAIL;
	}

	m_ActiveCamera = *iter;

	return S_OK;
}

CCameraObject* CCameraManager::GetCamera(const StringID& CameraID) const
{
	auto iter = m_Cameras.find(CameraID);
	if (iter == m_Cameras.end())
	{
		return nullptr;
	}
	auto pObj = CGameInstance::Get().GetGameObjectByHandle(iter->second);
	if (!pObj)
	{
		return nullptr;
	}

	if (!pObj->IsA(CCameraObject::StaticType))
	{
		return nullptr;
	}

	return static_cast<CCameraObject*>(pObj);
}

HRESULT CCameraManager::RegistCamera(const StringID& CameraID, const CHandle& handle)
{
	auto pObj = CGameInstance::Get().GetGameObjectByHandle(handle);
	if (!pObj)
	{
		return E_FAIL;
	}

	if (!pObj->IsA(CCameraObject::StaticType))
	{
		return E_FAIL;
	}

	auto iter = m_Cameras.find(CameraID);
	if (iter != m_Cameras.end())
	{
		m_Cameras.erase(iter);
	}
	m_Cameras.emplace(CameraID, handle);
	return S_OK;
}

//const CCameraObject* CCameraManager::GetCameraObject(const StringID& GroupID) const
//{
//	auto iter = m_ActiveCameras.find(GroupID);
//	if (iter == m_ActiveCameras.end())
//	{
//		return nullptr;
//	}
//
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(iter->second);
//	if (!pObj)
//	{
//		return nullptr;
//	}
//	
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return nullptr;
//	}
//
//	return static_cast<CCameraObject*>(pObj);
//}
//
//HRESULT CCameraManager::SetCameraObject(const StringID& GroupID, const CHandle& handle)
//{
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(handle);
//	if (!pObj)
//	{
//		return E_FAIL;
//	}
//
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return E_FAIL;
//	}
//
//	auto iter = m_ActiveCameras.find(GroupID);
//	if (iter != m_ActiveCameras.end())
//	{
//		m_ActiveCameras.erase(iter);
//	}
//	m_ActiveCameras.emplace(GroupID, handle);
//
//	return S_OK;
//}

//CCameraObject* CCameraManager::GetActiveGameCamera() const
//{
//	if (!m_ActiveGameCamera.has_value())
//	{
//		return nullptr;
//	}
//
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(m_ActiveGameCamera->second);
//	if (!pObj)
//	{
//		return nullptr;
//	}
//
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return nullptr;
//	}
//
//	return static_cast<CCameraObject*>(pObj);
//}
//
//HRESULT CCameraManager::SetActiveGameCamera(const StringID& CameraID)
//{
//	auto iter = m_GameCameras.find(CameraID);
//	if (iter == m_GameCameras.end())
//	{
//		return E_FAIL;
//	}
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(iter->second);
//	if (!pObj)
//	{
//		return E_FAIL;
//	}
//
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return E_FAIL;
//	}
//
//	m_ActiveGameCamera = *iter;
//
//	return S_OK;
//}
//
//CCameraObject* CCameraManager::GetActiveUICamera() const
//{
//	if (!m_ActiveUICamera.has_value())
//	{
//		return nullptr;
//	}
//
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(m_ActiveUICamera->second);
//	if (!pObj)
//	{
//		return nullptr;
//	}
//
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return nullptr;
//	}
//
//	return static_cast<CCameraObject*>(pObj);
//}
//
//HRESULT CCameraManager::SetActiveUICamera(const StringID& CameraID)
//{
//	auto iter = m_UICameras.find(CameraID);
//	if (iter == m_UICameras.end())
//	{
//		return E_FAIL;
//	}
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(iter->second);
//	if (!pObj)
//	{
//		return E_FAIL;
//	}
//
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return E_FAIL;
//	}
//
//	m_ActiveUICamera = *iter;
//
//	return S_OK;
//}
//
//CCameraObject* CCameraManager::GetActiveGameCamera(const StringID& CameraID) const
//{
//	auto tmp = GetActiveGameCamera();
//	if (tmp != GetGameCamera(CameraID))
//	{
//		return nullptr;
//	}
//	return tmp;
//}
//
//CCameraObject* CCameraManager::GetActiveUICamera(const StringID& CameraID) const
//{
//	auto tmp = GetActiveUICamera();
//	if (tmp != GetUICamera(CameraID))
//	{
//		return nullptr;
//	}
//	return tmp;
//}
//
//CCameraObject* CCameraManager::GetGameCamera(const StringID& CameraID) const
//{
//	auto iter = m_GameCameras.find(CameraID);
//	if (iter == m_GameCameras.end())
//	{
//		return nullptr;
//	}
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(iter->second);
//	if (!pObj)
//	{
//		return nullptr;
//	}
//
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return nullptr;
//	}
//
//	return static_cast<CCameraObject*>(pObj);
//}
//
//CCameraObject* CCameraManager::GetUICamera(const StringID& CameraID) const
//{
//	auto iter = m_UICameras.find(CameraID);
//	if (iter == m_UICameras.end())
//	{
//		return nullptr;
//	}
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(iter->second);
//	if (!pObj)
//	{
//		return nullptr;
//	}
//
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return nullptr;
//	}
//
//	return static_cast<CCameraObject*>(pObj);
//}
//
//HRESULT CCameraManager::RegistGameCamera(const StringID& CameraID, const CHandle& handle)
//{
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(handle);
//	if (!pObj)
//	{
//		return E_FAIL;
//	}
//
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return E_FAIL;
//	}
//
//	auto iter = m_GameCameras.find(CameraID);
//	if (iter != m_GameCameras.end())
//	{
//		m_GameCameras.erase(iter);
//	}
//	m_GameCameras.emplace(CameraID, handle);
//	return S_OK;
//}
//
//HRESULT CCameraManager::RegistUICamera(const StringID& CameraID, const CHandle& handle)
//{
//	auto pObj = CGameInstance::Get().GetGameObjectByHandle(handle);
//	if (!pObj)
//	{
//		return E_FAIL;
//	}
//
//	if (!pObj->IsA(CCameraObject::StaticType))
//	{
//		return E_FAIL;
//	}
//
//	auto iter = m_UICameras.find(CameraID);
//	if (iter != m_UICameras.end())
//	{
//		m_UICameras.erase(iter);
//	}
//	m_UICameras.emplace(CameraID, handle);
//	return S_OK;
//}

UPtr<CCameraManager> CCameraManager::Create()
{
	return ToUPtr(new CCameraManager{});
}
