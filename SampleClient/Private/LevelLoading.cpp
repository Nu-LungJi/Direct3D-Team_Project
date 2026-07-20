#include "pch.h"

#include "LevelLoading.h"
#include "GameInstance.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "LevelLogo.h"
#include "BackGround.h"
#include "LevelPlayground.h"
#include "Terrain.h"
#include "Particle.h"

#include "LevelUIEditor.h"
#include "LevelAnimEditor.h"
#include "LevelLightMap.h"
#include "LevelCollider.h"
#include "TestCollider.h"
#include "LevelPhysX.h"
#include "TestPhysX.h"
#include "TestPhysXTerrain.h"
#include "TestPhysXBox.h"
#include "TestPhysXBall.h"
#include "TestPhysXCapsule.h"
#include "LightObject.h"
#include "CTexUI.h"
#include "FlipBook.h"
#include "TextureUI.h"
#include "EffectUI.h"
#include "TextBox.h"
#include "Weapon.h"
#include "Button.h"
#include "SpellMeter.h"

#include "TestGob.h"

#include "TestCharacter.h"

#include "LevelLogoLoader.h"
#include "LevelPhysXLoader.h"
#include "LevelColliderLoader.h"
#include "LevelPlayGroundLoader.h"
#include "LevelAnimatorLoader.h"
#include "LevelLightMapLoader.h"
#include "LevelUIEditorLoader.h"

#include "UIManager.h"
#include "UICamera.h"
#include "FlyCamera.h"

NS_USING(Client)

CLevelLoading::CLevelLoading(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex) noexcept
	: CLevel{ ETOUI(LEVEL::LOADING) }
	, m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_eNextLevelIndex(eNextLevelIndex)
{
}

CLevelLoading::~CLevelLoading()
{
}

bool CLevelLoading::IsLevelChangeLocked() const
{
	return m_ePhase != PHASE::COMPLETE && m_ePhase != PHASE::FAILED;
}

HRESULT CLevelLoading::Initialize()
{
	const uint32_t iCurrentLevelID = Engine::CGameInstance::Get().GetCurrentLevelID();
	if (iCurrentLevelID != Engine::CLevel::INVALID_LEVEL_ID)
		m_ePreviousLevelIndex = static_cast<LEVEL>(iCurrentLevelID);

	Engine::CGameInstance::Get().GameObjectAllReset();

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
			E::CGameInstance::Get().SetActiveCamera("FLY");
		}
	}

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
				int x = 0;
			}

			// dynamic_cast vs static_cast benchmark 
			// dynamic_cast: 472ms, static_cast: 41ms
			if constexpr (false)
			{
				E::CGameObject* volatile val = E::CGameInstance::Get().GetGameObjectByHandle(uiCam.value());
				{
					auto start = std::chrono::high_resolution_clock::now();
					{
						E::CUICamera* volatile sink = nullptr;
						for (size_t i = 0; i < 10'000'000; ++i)
						{
							sink = dynamic_cast<E::CUICamera*>(val);
						}
					}
					auto end = std::chrono::high_resolution_clock::now();
					auto cost = std::chrono::duration<double, std::milli>(end - start).count();
					MSG_BOX_STR(std::to_wstring(cost).c_str());
				}
				{
					auto start = std::chrono::high_resolution_clock::now();
					{
						E::CUICamera* volatile sink = nullptr;
						for (size_t i = 0; i < 10'000'000; ++i)
						{
							if (val->IsA(E::CUICamera::StaticType))
							{
								sink = static_cast<E::CUICamera*>(val);
							}
							else
							{
								sink = nullptr;
							}
						}
					}
					auto end = std::chrono::high_resolution_clock::now();
					auto cost = std::chrono::duration<double, std::milli>(end - start).count();
					MSG_BOX_STR(std::to_wstring(cost).c_str());
				}
			}
			{
				const auto* t = E::CGameInstance::GetConst().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());

				auto* t2 = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());
			}
		}
	}

	return S_OK;
}

void CLevelLoading::Update(E::_float fTimeDelta)
{
	if (!m_bLoadUiResource)
	{
		std::optional<CHandle> hLoadingScreen = GET_SINGLE(UIManager)->LoadPrefab("LoadingDungeon1");
		m_bLoadUiResource = true;
	}
	GET_SINGLE(UIManager)->UpdateRootUIHandles();


	switch (m_ePhase)
	{
	case PHASE::READY:
		StartUnload();
		break;
	case PHASE::UNLOADING:
		CheckUnload();
		break;
	case PHASE::LOADING:
		CheckLoad();
		break;
	default:
		break;
	}
}

HRESULT CLevelLoading::Render()
{
	return S_OK;
}

void CLevelLoading::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevelLoading");
	ImGui::End();
}

void CLevelLoading::FrameEnd(E::_float fTimeDelta)
{
	if (m_bLoadEnd)
	{
		LoadEnd();
		return;
	}
}

HRESULT CLevelLoading::LoadEnd()
{
	Engine::UPtr<CLevel>	pNewLevel{};
	switch (m_eNextLevelIndex)
	{
	case LEVEL::LOGO:
		pNewLevel = CLevelLogo::Create();
		break;
	case LEVEL::PLAYGROUND:
		pNewLevel = CLevelPlayground::Create();
		break;
	case LEVEL::UIEDITOR:
		pNewLevel = CLevelUIEditor::Create();
		break;
	case LEVEL::ANIMEDITOR:
		pNewLevel = CLevelAnimEditor::Create();
		break;
	case LEVEL::COLLIDER:
		pNewLevel = CLevelCollider::Create();
		break;
	case LEVEL::LIGHTMAP:
		pNewLevel = CLevelLightMap::Create();
		break;
	case LEVEL::PHYSX:
		pNewLevel = CLevelPhysX::Create();
		break;
	}
	assert(pNewLevel);

	if (FAILED(Engine::CGameInstance::Get().ChangeLevel(std::move(pNewLevel))))
	{
		MSG_BOX("ChangeLevelFailed in loading");
		return E_FAIL;
	}
	return S_OK;
}

void CLevelLoading::StartUnload()
{
	if (!m_ePreviousLevelIndex || *m_ePreviousLevelIndex == LEVEL::LOADING)
	{
		StartLoad();
		return;
	}

	m_ePhase = PHASE::UNLOADING;
	switch (*m_ePreviousLevelIndex)
	{
	case LEVEL::LOGO:
		m_futUnloadFinish = CLevelLogoLoader::UnLoad();
		break;
	case LEVEL::PLAYGROUND:
		m_futUnloadFinish = CLevelPlayGroundLoader::UnLoad();
		break;
	case LEVEL::UIEDITOR:
		m_futUnloadFinish = CLevelUIEditorLoader::UnLoad();
		break;
	case LEVEL::ANIMEDITOR:
		m_futUnloadFinish = CLevelAnimatorLoader::UnLoad();
		break;
	case LEVEL::COLLIDER:
		m_futUnloadFinish = CLevelColliderLoader::UnLoad();
		break;
	case LEVEL::LIGHTMAP:
		m_futUnloadFinish = CLevelLightMapLoader::UnLoad();
		break;
	case LEVEL::PHYSX:
		m_futUnloadFinish = CLevelPhysXLoader::UnLoad();
		break;
	default:
		StartLoad();
		break;
	}
}

void CLevelLoading::CheckUnload()
{
	if (!m_futUnloadFinish.valid())
		return;

	if (m_futUnloadFinish.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	if (!m_futUnloadFinish.get())
	{
		m_ePhase = PHASE::FAILED;
		MSG_BOX("UNLOADING FAILED");
		return;
	}

	StartLoad();
}

void CLevelLoading::StartLoad()
{
	m_ePhase = PHASE::LOADING;
	switch (m_eNextLevelIndex)
	{
	case LEVEL::LOGO:
	{
		m_futLoadFinish = CLevelLogoLoader::Load();
	}
	break;
	case LEVEL::PLAYGROUND:
	{
		m_futLoadFinish = CLevelPlayGroundLoader::Load();
	}
	break;
	case LEVEL::UIEDITOR:
		m_futLoadFinish = CLevelUIEditorLoader::Load();
		break;
	case LEVEL::ANIMEDITOR:
	{
		m_futLoadFinish = CLevelAnimatorLoader::Load();
	}
		break;
	case LEVEL::COLLIDER:
	{
		m_futLoadFinish = CLevelColliderLoader::Load();
	}
	break;
	case LEVEL::LIGHTMAP:
	{
		m_futLoadFinish = CLevelLightMapLoader::Load();
	}
	break;
	case LEVEL::PHYSX:
	{
		m_futLoadFinish = CLevelPhysXLoader::Load();
	}
	break;
	default:
		m_ePhase = PHASE::COMPLETE;
		m_bLoadEnd = true;
		break;
	}

}

void CLevelLoading::CheckLoad()
{
	if (m_futLoadFinish.valid())
	{
		if (m_futLoadFinish.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			if (!m_futLoadFinish.get())
			{
				m_ePhase = PHASE::FAILED;
				MSG_BOX("LOADING FAILED");
				return;
			}

			m_ePhase = PHASE::COMPLETE;
			m_bLoadEnd = true;
		}
	}
}



Engine::UPtr<CLevelLoading> CLevelLoading::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex)
{
	auto	pInstance = Engine::UPtr<CLevelLoading>(new CLevelLoading(pDevice, pContext, eNextLevelIndex));

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelLoading");
		return nullptr;
	}

	return pInstance;
}

void CLevelLoading::Free()
{
	CLevel::Free();
}
