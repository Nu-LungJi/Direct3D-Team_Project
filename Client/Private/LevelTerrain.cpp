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
#include "Mon_Weapon.h"
#include "Client_Defines.h"
#include "OilBarrel.h"
#include "TmbGurdian.h"
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
	{
		for (uint32_t i = 0; i < 6; ++i)
		{
			COilBarrel::DESC desc{};
			desc.sObjectTag = "TestDynamic";
			desc.vInitialPosition = { 15.f, 55.f + (i * 3.f), 15.f };
			desc.vConvexScale = { 300.f, 300.f, 300.f };
			if (!E::CGameInstance::Get().AddGameObjectToLayer(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_OilBarrel,
				"03_PhysXTest",
				&desc))
				return E_FAIL;
		}
	}

	if (FAILED(SpawnPlayerCamera(SpawnPlayer())))
		return E_FAIL;


	

	

	

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
	{
		CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
		TmbGurdianDesc.sObjectTag = "TmbGurdian";
		TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::CHARLES_ROOKWOOD);
		XMStoreFloat3(&TmbGurdianDesc.vPos, XMVectorSet(44.f, 15.f, 65.f, 1.f));
		TmbGurdianDesc.ReSourceTag = "Model_Resource_TMBGurdian";
		TmbGurdianDesc.BeHaviorTag = "./Resources/json/BeHavior/GurDian3.json";
		TmbGurdianDesc.WeaponProtoName = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_Mace);
		TmbGurdianDesc.WeaponResourceName = "Model_Resource_Mace";
		XMStoreFloat3(&TmbGurdianDesc.vScale, XMVectorSet(2.f, 2.f, 2.f, 1));
		auto BossTmb = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, "02_TmbGurdian", &TmbGurdianDesc);

		if (!BossTmb)
		{
			MSG_BOX("Create TmbGurdian Failed in Rookwood");
			return E_FAIL;
		}
	}
	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);

	return S_OK;
}

void CLevelTerrain::Update(E::_float fTimeDelta)
{
	Picking();
}

HRESULT CLevelTerrain::Render()
{
	return S_OK;
}

void CLevelTerrain::UpdateGUI()
{
	ImGui::Begin("Terrain");
	//Resources();
	//Objects();
	//BeHaviors();

	//ImGui::Text("Select Resoruce : %s ", m_SelectResourceTag.c_str());
	//ImGui::Text("Select Object : %s ", m_SelectObjecteTag.c_str());
	//ImGui::Text("Select Behavior : %s ", m_SelectFileName.c_str());
	//m_fPos.y = 0.f;
	ImGui::Text("X :%2.f ", m_fPos.x); ImGui::SameLine(); ImGui::Text("Y : %2.f ", m_fPos.y); ImGui::SameLine(); ImGui::Text("Z : %2.f", m_fPos.z);

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

	ImGui::End();

}

void CLevelTerrain::Picking()
{
	if (auto pObj = CGameInstance::Get().GetActiveCamera())
	{
		const _float2 vMousePosition = CGameInstance::Get().GetMousePos();
		const _float2 vViewportSize = CGameInstance::Get().GetClientScreenSize();
		const auto& [ori, dir] = pObj->GetRayFromScreenPixel(vMousePosition, vViewportSize);
		PX_RAYCAST_RESULT rayResult{};
		if (CGameInstance::Get().GetPhysXManager()
			->RayCast(
				{
					.vOrigin = ori,
					.vDirection = dir,
					.fMaxDistance = 1000.f,
					.tFilter = {.iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC)}
				}
				, rayResult))
		{
			m_fPos = rayResult.vHitpos;
		}
	}
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

HRESULT CLevelTerrain::SpawnPlayerCamera(std::optional<CHandle> hPlayer)
{
	if (!hPlayer) return E_FAIL;
	CPlayerThirdPersonCamera::DESC Desc{};
	Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
	Desc.vAt = { 10.f, 50.f, 10.f };
	Desc.vEye = { 10.f, 53.f, 5.f };
	Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
	Desc.fFovY = 75.f;
	Desc.fNear = 0.1f;
	Desc.fFar = 1000.f;
	Desc.sObjectTag = "PlayerCamera";
	Desc.hTarget = hPlayer.value();

	auto hPlayerCamera = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::TERRAIN,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera,
		"101_CAMERA",
		&Desc);
	if (!hPlayerCamera || FAILED(E::CGameInstance::Get().RegistCamera(
		"PlayerCamera", *hPlayerCamera)))
	{
		return E_FAIL;
	}
	return S_OK;
}
std::optional<CHandle> CLevelTerrain::SpawnPlayer()
{
	CPlayer::DESC PlayerDesc{};
	PlayerDesc.sObjectTag = "Player";
	PlayerDesc.vInitialPosition = { 5.f, 100.f, 5.f };
	PlayerDesc.LevelTag = LEVEL::TERRAIN;
	return  E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::TERRAIN,
		PROTO_GAMEOBJECT::Prototype_GameObject_Player,
		"03_Player",
		&PlayerDesc);
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
