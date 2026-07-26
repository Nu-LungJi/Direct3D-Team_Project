#include "pch.h"
#include "LevelTerrain.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"
#include "Terrain.h"
#include "Particle.h"
#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "Weapon.h"
#include "Client_Defines.h"

NS_USING(Client)

CLevelTerrain::CLevelTerrain()
	:CLevel{ ETOUI(LEVEL::TERRAIN) }
{
}

CLevelTerrain::~CLevelTerrain()
{
}

HRESULT CLevelTerrain::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();

	{
		
		CGameObject::GAMEOBJECT_DESC Desc{};
		Desc.sObjectTag = "Terrain";
		if (auto h = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Terrain,
			"01_Terrain", &Desc))
		{
			int x = 0;
		}
	}
	/*{
		CHandle hPlayer{};
		CTerrain::DESC Desc{};
		Desc.sObjectTag = "Terrain";
		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer(m_strLevelName, "Prototype_GameObject_Terrain",
			"01_Terrain", &Desc))
		{
			int x = 0;
		}
		CTestPlayerCreatureEditor::DESC PlayerDesc{};
		PlayerDesc.sObjectTag = "TestPlayerCreatureEditor";
		PlayerDesc.vInitialPosition = { 10.f, 50.f, 10.f };
		auto hSpawnedPlayer = E::CGameInstance::Get().AddGameObjectToLayer(
			m_strLevelName,
			"Prototype_GameObject_TestPlayerCreatureEditor",
			"01_Player",
			&PlayerDesc);
		if (!hSpawnedPlayer)
			return E_FAIL;
		hPlayer = *hSpawnedPlayer;

		{
			CTestPlayer3CameraCreatureEditor::DESC Desc{};
			Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
			Desc.vAt = { 10.f, 50.f, 10.f };
			Desc.vEye = { 10.f, 53.f, 5.f };
			Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
			Desc.fFovY = 75.f;
			Desc.fNear = 0.1f;
			Desc.fFar = 1000.f;
			Desc.sObjectTag = "CREATURE_PLAYER_CAMERA";
			Desc.hTarget = hPlayer;

			auto hPlayerCamera = E::CGameInstance::Get().AddGameObjectToLayer(
				m_strLevelName,
				"Prototype_GameObject_TestPlayer3CameraCreatureEditor",
				"99_CAMERA",
				&Desc);
			if (!hPlayerCamera || FAILED(E::CGameInstance::Get().RegistCamera("CREATURE_PLAYER_CAMERA", *hPlayerCamera)))
			{
				return E_FAIL;
			}
		}
	}*/


	

	

	

	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::ORTHOGRAPHIC;
		Desc.fNear = 0.f;
		Desc.fFar = 1.f;
		Desc.fWidth = g_iWinSizeX;
		Desc.fHeight = g_iWinSizeY;
		Desc.sObjectTag = "UICam";
		Desc.vEye = { 0.f, 0.f, -0.1f };

		if (auto uiCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_UICamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("UI", uiCam.value())))
			{
				MSG_BOX("FailedToRegistCamera");
				return E_FAIL;
			}
		}
	}
	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { 0.f, 0.f, 0.f };
		Desc.vEye = { 0.f, 0.f, -5.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.1f;
		Desc.fFar = 100.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				int x = 0;
			}
			CGameInstance::Get().SetActiveCamera("FLY");
		}
	}

	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);

	return S_OK;
}

void CLevelTerrain::Update(E::_float fTimeDelta)
{
	//Picking();

	
}

HRESULT CLevelTerrain::Render()
{
	return S_OK;
}

void CLevelTerrain::UpdateGUI()
{
	//ImGui::Begin("Craeture Editor");
	//Resources();
	//Objects();
	//BeHaviors();

	//ImGui::Text("Select Resoruce : %s ", m_SelectResourceTag.c_str());
	//ImGui::Text("Select Object : %s ", m_SelectObjecteTag.c_str());
	//ImGui::Text("Select Behavior : %s ", m_SelectFileName.c_str());
	//m_fPos.y = 0.f;
	//ImGui::Text("X :%2.f ", m_fPos.x); ImGui::SameLine(); ImGui::Text("Y : %2.f ", m_fPos.y); ImGui::SameLine(); ImGui::Text("Z : %2.f", m_fPos.z);

	//if (ImGui::Button("Activate Med Debris Physics"))
	//{
	//	for (const CHandle& handle : m_MedDebrisHandles)
	//	{
	//		if (auto* debris = CGameInstance::Get()
	//			.GetGameObjectByHandleT<CMedDebris>(handle))
	//		{
	//			debris->RequestActivatePhysics();
	//		}
	//	}
	//}

	//if (!m_SelectResourceTag.empty() && !m_SelectObjecteTag.empty())
	//{
	//	if (ImGui::Button("SPAWN : "))
	//		m_bSpawn = !m_bSpawn;
	//	ImGui::SameLine(); ImGui::Text(m_bSpawn == true ? "TRUE" : "FALSE");

	//	const bool bGuiHovered = ImGui::IsWindowHovered(
	//		ImGuiHoveredFlags_AllowWhenBlockedByPopup);

	//	if (m_bSpawn && !bGuiHovered)
	//	{
	//		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	//		{

	//			CTestGob::MONSTER_DESC Desc{};
	//			Desc.sObjectTag = "Gobline";
	//			Desc.LevelTag = m_strLevelName;
	//			Desc.ReSourceTag = m_SelectResourceTag;
	//			Desc.BeHaviorTag = m_SelectFilePath;
	//			Desc.vPos = m_fPos;
	//			Desc.vPos.y += 50.f;
	//			auto Gobline = E::CGameInstance::Get().AddGameObjectToLayer(m_strLevelName, m_SelectObjecteTag, "02_Gobline", &Desc);
	//		}
	//	}


	//}

	//if (ImGui::TreeNode("Particle Test Monster"))
	//{

	//	CTestGob::MONSTER_DESC Desc{};

	//	Desc.bDonMove = true;
	//	Desc.sObjectTag = "Gobline";
	//	Desc.LevelTag = m_strLevelName;
	//	XMStoreFloat3(&Desc.vPos, XMVectorSet(0, 0, 0, 1));
	//	XMStoreFloat3(&Desc.vRot, XMVectorSet(0, 1, 0, 1));
	//	Desc.fAngle = 180.f;
	//	if (ImGui::Button("BOSS"))
	//	{
	//		Desc.ReSourceTag = "Model_Resource_TombProtector";
	//		Desc.BeHaviorTag = "./Resources/json/BeHavior/BossDef.json";
	//		XMStoreFloat3(&Desc.vScale, XMVectorSet(5.f, 5.f, 5.f, 1));

	//		auto Gobline = E::CGameInstance::Get().AddGameObjectToLayer(m_strLevelName, "Prototype_GameObject_Gobline", "02_Gobline", &Desc);

	//	}
	//	if (ImGui::Button("NORMAL"))
	//	{
	//		Desc.ReSourceTag = "Model_Resource_TombNormalProtector";
	//		Desc.BeHaviorTag = "./Resources/json/BeHavior/NormalDef.json";
	//		XMStoreFloat3(&Desc.vScale, XMVectorSet(2.f, 2.f, 2.f, 1));

	//		auto Gobline = E::CGameInstance::Get().AddGameObjectToLayer(m_strLevelName, "Prototype_GameObject_Gobline", "02_Gobline", &Desc);

	//	}
	//	ImGui::TreePop();
	//}

	//ImGui::End();

}

void CLevelTerrain::Picking()
{
	POINT tMouse{};
	GetCursorPos(&tMouse);
	ScreenToClient(g_hWnd, &tMouse);
	_float4x4 CameProj = {};
	_matrix   CamView = XMMatrixIdentity();
	XMStoreFloat4x4(&CameProj, XMMatrixIdentity());
	if (auto pCam = CGameInstance::Get().GetActiveCamera())
	{
		XMStoreFloat4x4(&CameProj, pCam->GetProj());
		CamView = pCam->GetView();
	}

	_float2 ViewPort = CGameInstance::Get().GetClientScreenSize();
	_float rayX = (2.f * tMouse.x / ViewPort.x - 1.f) / CameProj(0, 0);
	_float rayY = (-2.f * tMouse.y / ViewPort.y + 1.f) / CameProj(1, 1);

	//뷰포트에서의 광선 정의9
	_vector rayOrigin = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	_vector rayDir = XMVectorSet(rayX, rayY, 1.f, 0.f);

	//월드 좌표로 변환
	_matrix InverseView = XMMatrixInverse(nullptr, CamView);

	rayOrigin = XMVector3TransformCoord(rayOrigin, InverseView);
	rayDir = XMVector3Normalize(XMVector3TransformNormal(rayDir, InverseView));

	_float fMax = { FLT_MAX };
	_float tDis = 0;
	_float t1Dis = 0;
	_vector TriFirst[3]{ XMVectorSet(0,0,0,0),
						XMVectorSet(0,0,(129 * 129) * 6 ,0),
						XMVectorSet((129 * 129) * 6 ,0,(129 * 129) * 6,0) };

	_vector TriSecond[3]{ XMVectorSet((129 * 129) * 6,0,(129 * 129) * 6 ,0),
						  XMVectorSet((129 * 129) * 6,0,0,0),
						  XMVectorSet(0,0,0,0) };

	if (TriangleTests::Intersects(rayOrigin, rayDir, TriFirst[0], TriFirst[1], TriFirst[2], tDis))
		XMStoreFloat3(&m_fPos, rayOrigin + rayDir * tDis);

	if (TriangleTests::Intersects(rayOrigin, rayDir, TriSecond[0], TriSecond[1], TriSecond[2], t1Dis))
		XMStoreFloat3(&m_fPos, rayOrigin + rayDir * tDis);

}
void CLevelTerrain::Resources()
{

	ImGui::Text("RESOURCE "); ImGui::SameLine(70.f);
	if (ImGui::BeginCombo("##ReSource", m_SelectResourceTag.c_str()))
	{
		auto Resource = CGameInstance::Get().GetResource(m_strLevelName);
		for (const auto& [key, value] : Resource)
		{
			const _char* pName = key.str;
			_bool isSelected = m_SelectResourceTag == pName;
			ImGui::PushID(pName);
			if (ImGui::Selectable(pName, isSelected))
				m_SelectResourceTag = pName;

			if (isSelected)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
}
void CLevelTerrain::Objects()
{
	ImGui::Text("OBJECT "); ImGui::SameLine(70.f);
	if (ImGui::BeginCombo("##Object", m_SelectObjecteTag.c_str()))
	{
		for (const auto& key : CGameInstance::Get().GetPrototypeTags(m_strLevelName))
		{
			const _char* pName = key.str;
			_bool isSelected = m_SelectObjecteTag == pName;
			ImGui::PushID(pName);
			if (ImGui::Selectable(pName, isSelected))
				m_SelectObjecteTag = pName;

			if (isSelected)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}

}
void CLevelTerrain::BeHaviors()
{
	ImGui::Text("BEHAVIOR "); ImGui::SameLine(70.f);
	if (ImGui::BeginCombo("##Behavior", m_SelectFileName.c_str()))
	{
		auto& Prototypes = m_BeHaviorJsonList;
		for (const auto& [key, value] : Prototypes)
		{
			const _char* pName = key.c_str();
			_bool isSelected = m_SelectFileName == pName;
			ImGui::PushID(pName);
			if (ImGui::Selectable(pName, isSelected))
			{
				m_SelectFileName = key;
				m_SelectFilePath = value;
			}

			if (isSelected)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}

}
Engine::UPtr<CLevelTerrain> CLevelTerrain::Create()
{
	auto	pInstance = Engine::UPtr<CLevelTerrain>(new CLevelTerrain{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelTerrain");
	}

	return pInstance;
}


void CLevelTerrain::Free()
{
	CLevel::Free();
}
