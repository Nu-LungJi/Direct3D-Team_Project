#include "pch.h"

#include "CameraManager.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "CinematicSystem.h"
NS_USING(Engine)

CCameraManager::CCameraManager()
{
}

CCameraManager::~CCameraManager()
{
}

HRESULT CCameraManager::Initialize()
{
	CCameraObject::CAMERA_DESC CameraDesc{};
	CameraDesc.eProj = CCameraObject::PROJ::PERSPECTIVE;
	CameraDesc.vEye = { 0.f, 0.f, -5.f };
	CameraDesc.vAt = { 0.f, 0.f, 0.f };
	const auto vClientSize = CGameInstance::Get().GetClientScreenSize();
	CameraDesc.fAspect = vClientSize.x / vClientSize.y;
	CameraDesc.fFovY = 75.f;
	CameraDesc.fNear = 0.1f;
	CameraDesc.fFar = 1000.f;
	CameraDesc.sObjectTag = "CinematicCamera";

	const auto hCinematicCamera = CGameInstance::Get().AddGameObjectToLayer(
		ES_EngineProtoMajorType::CAMERAS,
		ES_EngineProtoGameObject::Prototype_GameObject_CinematicCamera,
		"00_ENGINE_CINEMATIC_CAMERA",
		&CameraDesc);
	if (!hCinematicCamera ||
		FAILED(RegistCamera("CinematicCamera", *hCinematicCamera)))
	{
		return E_FAIL;
	}

	m_pCinematicSystem = CCinematicSystem::Create(*this, "CinematicCamera");
	if (m_pCinematicSystem == nullptr)
		return E_FAIL;

	return S_OK;
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


UPtr<CCameraManager> CCameraManager::Create()
{
	auto pInstance = ToUPtr(new CCameraManager);
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}
