#include "pch.h"
#include "LevelUIEditor.h"
#include "GameInstance.h"
#include "LevelLoading.h"

#include "FlyCamera.h"
#include "UiCamera.h"
#include "ResCBuffer.h"
#include "ResTexture2D.h"
#include "CTexUI.h"
#include "UIObject.h"
#include "FlipBook.h"
#include <fstream>
#include "TextureUI.h"
#include "TextBox.h"
#include "TextUI.h"
#include "FlipbookUI.h"
#include "TextUI.h"
#include "EffectUI.h"

namespace fs = std::filesystem;

NS_USING(Client)

static int selectedParent = -1;

CLevelUIEditor::CLevelUIEditor()
{
}

CLevelUIEditor::~CLevelUIEditor()
{
}

HRESULT CLevelUIEditor::Initialize()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	E::CGameInstance::Get().GameObjectAllReset();

	Target_UI = std::nullopt;
	m_iEditorMode = 0;
	m_iButtonMode = 0;
	count = 0;

	m_vResTag.push_back("TEX_SHM");
	m_vResTag.push_back("TEX_MAP");
	m_vResTag.push_back("TEX_UI_T_NurtureMeterDiamond_Back_4k");
	m_vResTag.push_back("TEX_UI_T_NurtureMeterDiamond_Ready_4k");
	m_vResTag.push_back("TEX_UI_T_NurtureMeterDiamond_Outer_4k");

	m_vFlipBookResTag.push_back("Flipbook_LoadingWidget_Flame");
	m_vFlipBookResTag.push_back("Flipbook_LoadingWidget_Houses");
	m_vFlipBookResTag.push_back("Flipbook_VFXSmokeSim_D");
	m_vFlipBookResTag.push_back("Flipbook_VFX_T_ItemSpark_8x8_D");
	m_vFlipBookResTag.push_back("Flipbook_VFX_T_PopVFX_8x8_D");
	m_vFlipBookResTag.push_back("Flipbook_VFX_BlinkingStars");
	m_vFlipBookResTag.push_back("Flipbook_UI_T_MagicEffect1");
	m_vFlipBookResTag.push_back("Flipbook_UI_T_SmokeWispy_D");

	if (std::nullopt == Target_UI)
	{
		m_fX		= clientSize.x * 0.5f;
		m_fY		= clientSize.y * 0.5f;
		m_fSizeX	= 100.f;
		m_fSizeY	= 100.f;
		m_fAlpha	= 1.f;
		m_iWeight	= 0;
		strcpy_s(m_cName, "Name");
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

	{
		if (std::nullopt == Target_UI)
		{
			m_UIINFO.fX = clientSize.x * 0.5f;
			m_UIINFO.fY = clientSize.y * 0.5f;
			m_UIINFO.SizeX = 100.f;
			m_UIINFO.SizeY = 100.f;
			m_UIINFO.Alpha = 1.f;
			m_UIINFO.Weight = 0;
			m_UIINFO.Name = "None";

			strcpy_s(m_cName, "Name");
		}
	}

	return S_OK;
}

void CLevelUIEditor::Update(E::_float fTimeDelta)
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	_bool bP = CGameInstance::Get().KeyDown(DIK_P);
	_bool bC = CGameInstance::Get().KeyPressing(DIK_C);
	_bool bV = CGameInstance::Get().KeyPressing(DIK_V);
	_bool bDelete = CGameInstance::Get().KeyDown(DIK_DELETE);

	//const _tchar* text = L"Test";
	//CGameInstance::Get().FontAddLateDraw(RENDERGROUP::UI, "NeoDGM_15px", text, { clientSize.x * 0.5f, clientSize.y * 0.5f });

	if (bP)
	{
		// debug용 
		if(false)
		{
			count++;
			CUIObject::UIOBJECT_DESC Desc{};
			Desc.sObjectTag = "UI_" + std::to_string(count);
			Desc.fSizeX = 200.f;
			Desc.fSizeY = 200.f;
			Desc.fX = clientSize.x * 0.5f;
			Desc.fY = clientSize.y * 0.5f;
			Desc.fAlpha = 1.f;
			Desc.ResTag = "TEX_MAP";
			Desc.ResWeight = count;
			Desc.UIType = ETOUI(UI_TYPE::TEXUI);

			std::optional<CHandle> handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		}

		if (false)
		{
			count++;
			CUIObject::UIOBJECT_DESC Desc{};
			Desc.sObjectTag = "UI_" + std::to_string(count);
			Desc.fSizeX = 200.f;
			Desc.fSizeY = 200.f;
			Desc.fX = clientSize.x * 0.5f;
			Desc.fY = clientSize.y * 0.5f;
			Desc.fAlpha = 1.f;
			Desc.ResTag = "TEX_MAP";
			Desc.ResWeight = count;
			Desc.UIType = ETOUI(UI_TYPE::TEXUI);

			std::optional<CHandle> handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		}

		if (true)
		{
			count++;
			CTextUI::TEXT_DESC desc{};

			desc.fSizeX = 2.f;
			desc.fSizeY = 2.f;
			desc.fX = clientSize.x * 0.5f;
			desc.fY = clientSize.y * 0.5f;
			desc.fAlpha = 0.05f;
			desc.Text = L"Test";

			std::optional<CHandle> handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextBox", "Layer_UI", &desc);
		}
	}
	if (bV)
	{
		if (std::nullopt != m_oSelectHandle)
		{
			Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_oSelectHandle);
			selectUI->SetPendingDestroyCascade();

			m_oSelectHandle = std::nullopt;
		}
		m_iButtonMode = ETOUI(UiButtonMode::SELECT);
	}
	else if (bC)
	{
		Target_UI = std::nullopt;
		m_iButtonMode = ETOUI(UiButtonMode::CREATE);
	}
	else
		m_iButtonMode = ETOUI(UiButtonMode::DEFAULT);

	// Default
	if (bDelete)
	{
		if (std::nullopt != m_oSelectHandle)
		{
			Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_oSelectHandle);
			selectUI->SetPendingDestroyCascade();

			m_oSelectHandle = std::nullopt;
		}

		if (std::nullopt != Target_UI)
		{
			DeleteUIRecursive(Target_UI);

			Target_UI = std::nullopt;
		}
	}

	if (std::nullopt != m_oSelectHandle)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_oSelectHandle);
		UI_INFO& selectInfo = selectUI->GetUIInfo();
		selectInfo.SizeX = m_UIINFO.SizeX;
		selectInfo.SizeY = m_UIINFO.SizeY;
		selectUI->CalcUICoord();
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		UI_INFO& selectInfo = selectUI->GetUIInfo();

		selectInfo.fX = m_UIINFO.fX;
		selectInfo.fY = m_UIINFO.fY;
		selectInfo.SizeX = m_UIINFO.SizeX;
		selectInfo.SizeY = m_UIINFO.SizeY;
		selectInfo.Alpha = m_UIINFO.Alpha;
		selectInfo.Weight = m_UIINFO.Weight;
		selectInfo.Name = m_cName;
		selectInfo.Color = m_UIINFO.Color;

		if (ETOUI(UI_TYPE::FLIPBOOK) == selectUI->GetUIType())
		{
			FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(selectUI)->GetFlipInfo();

			m_FLIPINFO.cellsize = flipInfo.cellsize;
			m_FLIPINFO.Duration = flipInfo.Duration;
			m_FLIPINFO.TotalFrame = flipInfo.TotalFrame;
			m_FLIPINFO.Padding = flipInfo.Padding;
		}
		selectUI->CalcUICoord();
	}

	switch (m_iButtonMode)
	{
	case ETOUI(UiButtonMode::DEFAULT):
		break;
	case ETOUI(UiButtonMode::CREATE):
		CreateMode();
		break;
	case ETOUI(UiButtonMode::SELECT):
		SelectMode();
		break;
	default:
		break;
	}
}

HRESULT CLevelUIEditor::Render()
{
	return S_OK;
}

void CLevelUIEditor::UpdateGUI()
{

	switch (m_iEditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE):
		ArrangeMode();
		break;
	case ETOUI(UiEditorMode::PREFAB):
		PrefabMode();
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		FlipbookMode();
		break;
	default:
		break;
	}
}

void CLevelUIEditor::FrameStart(E::_float fTimeDelta)
{
}

void CLevelUIEditor::CreateMode()
{
	_bool MouseLB = CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB);

	if (std::nullopt != m_oSelectHandle)
	{
		if (MouseLB)
		{
			CTextureUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*m_oSelectHandle);
			const UI_INFO& selectInfo = selectUI->GetUIInfo();
			_float2 mousePos = E::CGameInstance::Get().GetMousePos();

			CTextureUI::UIOBJECT_DESC Desc{};

			count++;
			Desc.sObjectTag = "UI_" + std::to_string(count);
			Desc.Name = "UI_" + std::to_string(count);
			Desc.fSizeX = selectInfo.SizeX;
			Desc.fSizeY = selectInfo.SizeY;
			Desc.fX = mousePos.x;
			Desc.fY = mousePos.y;
			Desc.fAlpha = 1.f;
			Desc.ResTag = selectInfo.Restag;
			Desc.UIType = ETOUI(UI_TYPE::TEXUI);
			Desc.ResWeight = count;

			E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		}
	}
}

void CLevelUIEditor::SelectMode()
{
	_bool MouseLB = CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB);
	_bool MouseLBPressing = CGameInstance::Get().MousePressing(MOUSEKEYSTATE::LB);

	if (MouseLB)
	{
		if (m_iEditorMode == ETOUI(UiEditorMode::ARRANGE))
			PickingOnlyRoot();
		else
			Picking();
	}
		

	if (MouseLBPressing && std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		UI_INFO& selectInfo = selectUI->GetUIInfo();

		_float2 mousePos = CGameInstance::Get().GetMousePos();

		m_UIINFO.fX = mousePos.x - m_vDragOffset.x;
		m_UIINFO.fY = mousePos.y - m_vDragOffset.y;

		if (std::nullopt != selectUI->GetParent())
		{
			Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*selectUI->GetParent());
			UI_INFO& parentInfo = parentUI->GetUIInfo();

			selectInfo.LocalX = m_UIINFO.fX - parentInfo.fX;
			selectInfo.LocalY = m_UIINFO.fY - parentInfo.fY;
		}
		else
		{
			selectInfo.fX = m_UIINFO.fX;
			selectInfo.fY = m_UIINFO.fY;
		}
		selectUI->CalcUICoord();
	}
}

void CLevelUIEditor::ArrangeMode()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	
	_float2 mousePos = CGameInstance::Get().GetMousePos();

	DrawJsonFileLoader(m_iEditorMode);

	ImGui::Begin("EDITOR_MODE: ARRANGE_MODE");

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Save / Load");
	ImGui::Separator();

	if (ImGui::Button("Save"))
		Save();

	ImGui::SameLine();

	if (ImGui::Button("Load"))
		Load();

	ImGui::SetNextItemWidth(100);
	ImGui::InputText("LevelName", m_cLevelName, sizeof(m_cLevelName));

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Mode");
	ImGui::Separator();

	if (ImGui::Button("ARRAGE_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);
		RefreshJsonFileList();
	}
	ImGui::SameLine();
	if (ImGui::Button("PREFAB_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::PREFAB);
		RefreshJsonFileList();
	}
	ImGui::SameLine();
	if (ImGui::Button("FLIPBOOK_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::FLIPBOOK);
		RefreshJsonFileList();
	}

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Target_State");
	ImGui::Separator();

	ImGui::Text("PosX  : %.2f  ", m_UIINFO.fX);
	ImGui::SameLine(150);
	ImGui::Text("PosY   : %.2f", m_UIINFO.fY);

	ImGui::Text("SizeX : %.2f  ", m_UIINFO.SizeX);
	ImGui::SameLine(150);
	ImGui::Text("SizeY  : %.2f", m_UIINFO.SizeY);

	ImGui::Text("Alpha : %.2f", m_UIINFO.Alpha);
	ImGui::SameLine(150);
	ImGui::Text("Weight : %.d", m_UIINFO.Weight);

	ImGui::Text("Name : %s", m_cName);

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Input_State");
	ImGui::Separator();

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fx", &m_UIINFO.fX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fy", &m_UIINFO.fY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeX", &m_UIINFO.SizeX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeY", &m_UIINFO.SizeY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fAlpha", &m_UIINFO.Alpha, 0.001f, 0.0f, 1.f);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragInt("iWeight", &m_UIINFO.Weight, 1.f, 0.0f, 100);

	ImGui::SetNextItemWidth(80);
	ImGui::InputText("Name", m_cName, sizeof(m_cName));

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Level");
	ImGui::Separator();

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Images");
	ImGui::Separator();

	if (ImGui::BeginTable("TextureTable", 2))
	{
		for (size_t i = 0; i < m_vResTag.size(); ++i)
		{
			ImGui::TableNextColumn();

			ImGui::PushID((int)i);

			const auto& srv = E::CGameInstance::GetConst()
				.GetResourceFirst<E::CResTexture2D>("LEVEL_UIEDITOR", m_vResTag[i]);

			if (ImGui::ImageButton((ImTextureID)srv->GetSRV().Get(), ImVec2(100, 100)))
			{
				if (std::nullopt != m_oSelectHandle)
				{
					CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_oSelectHandle);
					selectUI->SetPendingDestroyCascade();

					m_oSelectHandle = std::nullopt;
				}

				CTextureUI::UIOBJECT_DESC Desc{};

				Desc.sObjectTag = "Select_Image";
				Desc.fSizeX = m_UIINFO.SizeX;
				Desc.fSizeY = m_UIINFO.SizeY;
				Desc.fX = g_iWinSizeX * 0.5f;
				Desc.fY = g_iWinSizeY * 0.5f;
				Desc.fAlpha = m_UIINFO.Alpha * 0.3f;
				Desc.ResTag = m_vResTag[i];
				Desc.UIType = ETOUI(UI_TYPE::TEXUI);
				Desc.ResWeight = 10000;

				m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI","Layer_UI_Texture", &Desc);
				CTextureUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*m_oSelectHandle);
				selectUI->SetMouseTracking(true);
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	ImGui::End();
}  

void CLevelUIEditor::PrefabMode()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	_float2 mousePos = CGameInstance::Get().GetMousePos();

	DrawJsonFileLoader(m_iEditorMode);

	ImGui::Begin("EDITOR_MODE: ARRANGE_MODE");

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Save / Load");
	ImGui::Separator();
	
	if (ImGui::Button("Save"))
		PrefabSave();
	
	ImGui::SameLine();
	
	if (ImGui::Button("Load"))
		PrefabLoad();

	ImGui::SetNextItemWidth(100);
	ImGui::InputText("PrefabName", m_cPrefabName, sizeof(m_cPrefabName));

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Mode");
	ImGui::Separator();

	if (ImGui::Button("ARRAGE_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);
		RefreshJsonFileList();
	}
	ImGui::SameLine();
	if (ImGui::Button("PREFAB_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::PREFAB);
		RefreshJsonFileList();
	}
	ImGui::SameLine();
	if (ImGui::Button("FLIPBOOK_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::FLIPBOOK);
		RefreshJsonFileList();
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		m_UIINFO.Name = targetUI->GetName();
		strcpy_s(m_cName, sizeof(m_cName), m_UIINFO.Name.c_str());

		if (std::nullopt != targetUI->GetParent())
			LocalStateView();
		else
			StateView();
	}
	else
		StateView();
	
	std::vector<Engine::CUIObject*> uiList;

	if (nullptr != CGameInstance::Get().GetGameObjectLayer("Layer_UI") && (std::nullopt != Target_UI))
	{
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0, 1, 1, 1), "Set_Parent");
		ImGui::Separator();

		std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

		for (auto ui : uiHandles)
		{
			Engine::CUIObject* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);

			if (nullptr == checkUI)
				continue;

			if (ui == Target_UI)   // 자기 자신은 Parent가 될 수 없음
				continue;

			uiList.push_back(checkUI);
		}
		const char* preview = "None";

		Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);

		if (selectedParent >= 0)
			preview = uiList[selectedParent]->GetName();

		if (ImGui::BeginCombo("Parent", preview))
		{
			bool selected = (selectedParent == -1);

			if (ImGui::Selectable("None", selected))
			{
				selectedParent = -1;
				targetUI->SetParent(std::nullopt);
			}

			for (int i = 0; i < uiList.size(); ++i)
			{
				bool isSelected = (selectedParent == i);

				if (ImGui::Selectable(uiList[i]->GetName(), isSelected))
				{
					selectedParent = i;

					targetUI->SetParent(uiList[i]->GetHandle());

					Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(uiList[i]->GetHandle());
					parentUI->AddChildren(*Target_UI);


					UI_INFO& targetInfo = targetUI->GetUIInfo();
					UI_INFO& parentInfo = parentUI->GetUIInfo();

					targetInfo.LocalX = m_UIINFO.fX - parentInfo.fX;
					targetInfo.LocalY = m_UIINFO.fY - parentInfo.fY;

					targetUI->CalcUICoord();
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
	}

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Images");
	ImGui::Separator();

	if (ImGui::BeginTable("TextureTable", 2))
	{
		for (size_t i = 0; i < m_vResTag.size(); ++i)
		{
			ImGui::TableNextColumn();

			ImGui::PushID((int)i);

			const auto& srv = E::CGameInstance::GetConst()
				.GetResourceFirst<E::CResTexture2D>("LEVEL_UIEDITOR", m_vResTag[i]);

			if (ImGui::ImageButton((ImTextureID)srv->GetSRV().Get(), ImVec2(100, 100)))
			{
				if (std::nullopt != m_oSelectHandle)
				{
					CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_oSelectHandle);
					selectUI->SetPendingDestroyCascade();

					m_oSelectHandle = std::nullopt;
				}

				CTextureUI::UIOBJECT_DESC Desc{};

				Desc.sObjectTag = "Select_Image";
				Desc.fSizeX = m_UIINFO.SizeX;
				Desc.fSizeY = m_UIINFO.SizeY;
				Desc.fX = g_iWinSizeX * 0.5f;
				Desc.fY = g_iWinSizeY * 0.5f;
				Desc.fAlpha = m_UIINFO.Alpha * 0.3f;
				Desc.ResTag = m_vResTag[i];
				Desc.UIType = ETOUI(UI_TYPE::TEXUI);
				Desc.ResWeight = 10000;

				m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI_Texture", &Desc);
				CTextureUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*m_oSelectHandle);
				selectUI->SetMouseTracking(true);
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	ImGui::End();
}

void CLevelUIEditor::FlipbookMode()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	_float2 mousePos = CGameInstance::Get().GetMousePos();

	DrawJsonFileLoader(m_iEditorMode);

	ImGui::Begin("EDITOR_MODE: ARRANGE_MODE");

	//if (ImGui::Button("Save"))
	//	PrefabSave();
	//
	//ImGui::SameLine();
	//
	//if (ImGui::Button("Load"))
	//	PrefabLoad();

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Mode");
	ImGui::Separator();

	if (ImGui::Button("ARRAGE_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);
		RefreshJsonFileList();
	}
	ImGui::SameLine();
	if (ImGui::Button("PREFAB_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::PREFAB);
		RefreshJsonFileList();
	}
	ImGui::SameLine();
	if (ImGui::Button("FLIPBOOK_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::FLIPBOOK);
		RefreshJsonFileList();
	}
		

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Save FlipBook");
	ImGui::Separator();

	ImGui::SetNextItemWidth(100);
	ImGui::InputText("Save_FlipbookName", m_cPrefabName, sizeof(m_cPrefabName));

	if (ImGui::Button("Save_Flipbook"))
		PrefabSave();

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Make FlipBook");
	ImGui::Separator();

	ImGui::SetNextItemWidth(100);
	ImGui::InputText("FlipbookName", m_cPrefabName, sizeof(m_cPrefabName));

	if (ImGui::Button("Make_Flipbook"))
	{
		FlipBookMake();
	}

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Animation_Value");
	ImGui::Separator();

	ImGui::SetNextItemWidth(80);
	ImGui::InputFloat("CellSize", &m_fCellSize);
	ImGui::SetNextItemWidth(80);
	ImGui::InputFloat("Duration", &m_fDuration);
	ImGui::SetNextItemWidth(80);
	ImGui::InputInt("TotalFrame", &m_iTotalFrame);

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);

		static_cast<CFlipBook*>(selectUI)->SetCellSize(m_fCellSize);
		static_cast<CFlipBook*>(selectUI)->SetDuration(m_fDuration);
		static_cast<CFlipBook*>(selectUI)->SetTotalFrame(m_iTotalFrame);

		if (std::nullopt != selectUI->GetParent())
			LocalStateView();
		else
			StateView();
	}
	else
		StateView();

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Images");
	ImGui::Separator();

	if (ImGui::BeginTable("TextureTable", 2))
	{
		for (size_t i = 0; i < m_vFlipBookResTag.size(); ++i)
		{
			ImGui::TableNextColumn();

			ImGui::PushID((int)i);

			const auto& srv = E::CGameInstance::GetConst()
				.GetResourceFirst<E::CResTexture2D>("LEVEL_UIEDITOR", m_vFlipBookResTag[i]);

			if (ImGui::ImageButton((ImTextureID)srv->GetSRV().Get(), ImVec2(100, 100)))
			{
				strcpy_s(m_cResTag, sizeof(m_cResTag), m_vFlipBookResTag[i].c_str());
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	ImGui::End();
}

void CLevelUIEditor::Picking()
{
	_float2 mousePos = CGameInstance::Get().GetMousePos();

	std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	Target_UI = std::nullopt;

	uint32_t maxWeight = 0;

	for (auto ui : uiHandles)
	{
		Engine::CUIObject* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);

		if (nullptr == checkUI)
			continue;

		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);
		const UI_INFO& selectInfo = selectUI->GetUIInfo();

		_float2 origin = { selectInfo.fX, selectInfo.fY };
		_float2 size = { selectInfo.SizeX, selectInfo.SizeY };

		_float2 minPos =
		{
			origin.x - size.x * 0.5f,
			origin.y - size.y * 0.5f
		};

		_float2 maxPos =
		{
			origin.x + size.x * 0.5f,
			origin.y + size.y * 0.5f
		};

		if (mousePos.x >= minPos.x &&
			mousePos.x <= maxPos.x &&
			mousePos.y >= minPos.y &&
			mousePos.y <= maxPos.y)
		{
			uint32_t curWeight = selectInfo.Weight;
			if (curWeight >= maxWeight)
			{
				maxWeight = curWeight;
				Target_UI = ui;

				m_vDragOffset = { CGameInstance::Get().GetMousePos().x - origin.x,
					CGameInstance::Get().GetMousePos().y - origin.y };
			}
		}
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		UI_INFO& selectInfo = selectUI->GetUIInfo();
		m_UIINFO.fX = selectInfo.fX;
		m_UIINFO.fY = selectInfo.fY;
		m_UIINFO.SizeX = selectInfo.SizeX;
		m_UIINFO.SizeY = selectInfo.SizeY;
		m_UIINFO.Alpha = selectInfo.Alpha;
		m_UIINFO.Weight = selectInfo.Weight;
		strcpy_s(m_cName, sizeof(m_cName), selectInfo.Name.c_str());

		if (ETOUI(UI_TYPE::FLIPBOOK) == selectInfo.UIType)
		{
			FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(selectUI)->GetFlipInfo();

			m_FLIPINFO.cellsize = flipInfo.cellsize;
			m_FLIPINFO.Duration = flipInfo.Duration;
			m_FLIPINFO.TotalFrame = flipInfo.TotalFrame;
			m_FLIPINFO.Padding = flipInfo.Padding;
		}
	}
}

void CLevelUIEditor::PickingOnlyRoot()
{
	_float2 mousePos = CGameInstance::Get().GetMousePos();

	std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	Target_UI = std::nullopt;

	uint32_t maxWeight = 0;

	for (auto ui : uiHandles)
	{
		Engine::CUIObject* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);

		if (nullptr == checkUI)
			continue;

		if (std::nullopt != checkUI->GetParent())
			continue;

		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);
		const UI_INFO& selectInfo = selectUI->GetUIInfo();

		_float2 origin = { selectInfo.fX, selectInfo.fY };
		_float2 size = { selectInfo.SizeX, selectInfo.SizeY};

		_float2 minPos =
		{
			origin.x - size.x * 0.5f,
			origin.y - size.y * 0.5f
		};

		_float2 maxPos =
		{
			origin.x + size.x * 0.5f,
			origin.y + size.y * 0.5f
		};

		if (mousePos.x >= minPos.x &&
			mousePos.x <= maxPos.x &&
			mousePos.y >= minPos.y &&
			mousePos.y <= maxPos.y)
		{
			uint32_t curWeight = selectInfo.Weight;
			if (curWeight >= maxWeight)
			{
				maxWeight = curWeight;
				Target_UI = ui;

				m_vDragOffset = { CGameInstance::Get().GetMousePos().x - origin.x,
					CGameInstance::Get().GetMousePos().y - origin.y };
			}
		}
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		UI_INFO& selectInfo = selectUI->GetUIInfo();
		m_UIINFO.fX = selectInfo.fX;
		m_UIINFO.fY = selectInfo.fY;
		m_UIINFO.SizeX = selectInfo.SizeX;
		m_UIINFO.SizeY = selectInfo.SizeY;
		m_UIINFO.Alpha = selectInfo.Alpha;
		m_UIINFO.Weight = selectInfo.Weight;
		strcpy_s(m_cName, sizeof(m_cName), selectInfo.Name.c_str());

		if (ETOUI(UI_TYPE::FLIPBOOK) == selectInfo.UIType)
		{
			FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(selectUI)->GetFlipInfo();

			m_FLIPINFO.cellsize = flipInfo.cellsize;
			m_FLIPINFO.Duration = flipInfo.Duration;
			m_FLIPINFO.TotalFrame = flipInfo.TotalFrame;
			m_FLIPINFO.Padding = flipInfo.Padding;
		}
	}
}

void CLevelUIEditor::Save()
{
	std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	nlohmann::ordered_json root;

	root["LevelName"] = m_cLevelName;
	root["UI"] = nlohmann::ordered_json::array();

	for (CHandle handle : uiHandles)
	{
		CTexUI* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(handle);

		if (pUI == nullptr)
			continue;

		if (pUI->GetParent().has_value())
			continue;

		nlohmann::ordered_json obj;
		SaveUIRecursive(pUI, obj);

		root["UI"].push_back(obj);
	}

	char path[256] = "./Resources/SampleClient/UIData/LevelUI/";
	strcat_s(path, sizeof(path), m_cLevelName);
	char final[256] = ".json";
	strcat_s(path, sizeof(path), final);

	std::ofstream file(path);
	if (!file.is_open())
	{
		MSG_BOX("파일 저장 실패");
		return;
	}
	else
	{
		MSG_BOX("파일 저장 성공");
	}
	file << root.dump(4);

	file.close();
}

void CLevelUIEditor::Load()
{
	char path[256] = "./Resources/SampleClient/UIData/LevelUI/";
	strcat_s(path, sizeof(path), m_cLevelName);
	strcat_s(path, sizeof(path), ".json");

	std::ifstream file(path);

	if (!file.is_open())
	{
		MSG_BOX("파일 열기 실패");
		return;
	}

	nlohmann::ordered_json root;
	file >> root;
	file.close();

	for (const auto& obj : root["UI"])
	{
		LoadUIRecursive(obj, nullptr);
	}

}

void CLevelUIEditor::PrefabSave()
{
	std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	nlohmann::ordered_json root;

	root["PrefabName"] = m_cPrefabName;
	root["UI"] = nlohmann::ordered_json::array();

	for (CHandle handle : uiHandles)
	{
		E::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUIObject>(handle);

		if (pUI == nullptr)
			continue;

		if (pUI->GetParent().has_value())
			continue;

		nlohmann::ordered_json obj;
		SaveUIRecursive(pUI, obj);

		root["UI"].push_back(obj);
	}

	switch (m_iEditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/LevelUI/");
		break;
	case ETOUI(UiEditorMode::PREFAB):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/Prefabs/");
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/FlipBook/");
		break;
	}
	
	char path[256] = "";
	strcpy_s(path, sizeof(path), g_BasePath);
	strcat_s(path, sizeof(path), m_cPrefabName);
	char final[256] = ".json";
	strcat_s(path, sizeof(path), final);

	std::ofstream file(path);
	if (!file.is_open())
	{
		MSG_BOX("파일 저장 실패");
		return;
	}
	else
	{
		MSG_BOX("파일 저장 성공");
	}
	file << root.dump(4);

	file.close();
}

void CLevelUIEditor::PrefabLoad()
{
	switch (m_iEditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/LevelUI/");
		break;
	case ETOUI(UiEditorMode::PREFAB):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/Prefabs/");
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/FlipBook/");
		break;
	}

	char path[256] = "";
	strcpy_s(path, sizeof(path), g_BasePath);
	strcat_s(path, sizeof(path), m_cPrefabName);
	strcat_s(path, sizeof(path), ".json");

	std::ifstream file(path);

	if (!file.is_open())
	{
		MSG_BOX("파일 열기 실패");
		return;
	}

	nlohmann::ordered_json root;
	file >> root;
	file.close();

	for (const auto& obj : root["UI"])
	{
		LoadUIRecursive(obj, nullptr);
	}
}

void CLevelUIEditor::FlipBookMake()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	count++;
	CFlipbookUI::FLIPBOOK_DESC Desc{};
	Desc.sObjectTag = "UI_" + std::to_string(count);
	Desc.fSizeX = 200.f;
	Desc.fSizeY = 200.f;
	Desc.fX = clientSize.x * 0.5f;
	Desc.fY = clientSize.y * 0.5f;
	Desc.fAlpha = 1.f;
	Desc.ResTag = m_UIINFO.Restag;
	Desc.ResWeight = count;
	Desc.UIType = ETOUI(UI_TYPE::FLIPBOOK);

	std::optional<CHandle> handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_EffectUI", "Layer_UI", &Desc);
}

void CLevelUIEditor::SaveUIRecursive(E::CUIObject* pUI, nlohmann::ordered_json& obj)
{
	UI_INFO& uiInfo = pUI->GetUIInfo();

	obj["UiType"] = uiInfo.UIType;
	obj["UI_EFFECT_TYPE"] = uiInfo.EffectType;

	obj["Name"] = uiInfo.Name;

	obj["X"] = uiInfo.fX;
	obj["Y"] = uiInfo.fY;

	obj["LocalX"] = uiInfo.LocalX;
	obj["LocalY"] = uiInfo.LocalY;

	obj["SizeX"] = uiInfo.SizeX;
	obj["SizeY"] = uiInfo.SizeY;

	obj["WidthRatioX"] = uiInfo.WidthRatioX;
	obj["WidthRatioY"] = uiInfo.WidthRatioY;
	
	obj["Rot"] = uiInfo.Rot;
	obj["LocalRot"] = uiInfo.LocalRot;

	obj["Alpha"] = uiInfo.Alpha;
	obj["AlphaRatio"] = uiInfo.AlphaRatio;

	obj["Weight"] = uiInfo.Weight;
	obj["WeightOffset"] = uiInfo.WeightOffset;

	obj["ResTag"] = uiInfo.Restag;

	obj["Color"] = { uiInfo.Color.x, uiInfo.Color.y, uiInfo.Color.z };

	switch (uiInfo.UIType)
	{
	case ETOUI(UI_TYPE::TEXUI):
		break;
	case ETOUI(UI_TYPE::FLIPBOOK):
	{
		const FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(pUI)->GetFlipInfo();
		obj["CellSize"] = flipInfo.cellsize;
		obj["TotalFrame"] = flipInfo.TotalFrame;
		obj["Padding"] = flipInfo.Padding;
		obj["Duration"] = flipInfo.Duration;
		break;
	}
	case ETOUI(UI_TYPE::TEXT):
	{
		const TEXT_INFO& textInfo = static_cast<CTextUI*>(pUI)->GetTextInfo();
		//obj["Text"] = textInfo.Text;
	}
	default:
		break;
	}

	obj["Children"] = nlohmann::ordered_json::array();

	for (CHandle childHandle : pUI->GetChildren())
	{
		E::CUIObject* pChild = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUIObject>(childHandle);

		if (pChild == nullptr)
			continue;

		nlohmann::ordered_json childObj;
		SaveUIRecursive(pChild, childObj);

		obj["Children"].push_back(childObj);
	}
}

E::CUIObject* CLevelUIEditor::LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent)
{
	int uiType = obj["UiType"];
	count++;
	E::CUIObject* pUI = nullptr;

	E::CUIObject::UIOBJECT_DESC Desc{};
	std::optional<CHandle> uiHandle = std::nullopt;

	Desc.sObjectTag = obj["Name"];

	switch (uiType)
	{
	case ETOUI(UI_TYPE::TEXUI):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::FLIPBOOK):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_EffectUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CEffectUI>(*uiHandle);
		{
			FLIP_INFO& flipInfo = static_cast<CEffectUI*>(pUI)->GetFlipInfo();
			flipInfo.cellsize	= obj["CellSize"];
			flipInfo.TotalFrame = obj["TotalFrame"];
			flipInfo.Padding	= obj["Padding"];
			flipInfo.Duration	= obj["Duration"];
		}
		break;
	case ETOUI(UI_TYPE::TEXT):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextBox", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*uiHandle);
		{
			TEXT_INFO& textInfo = static_cast<CTextBox*>(pUI)->GetTextInfo();
			//textInfo.Text = obj["Text"];
		}
		
	default:
		break;
	}

	UI_INFO& uiInfo = static_cast<CUIObject*>(pUI)->GetUIInfo();

	if (pUI == nullptr)
		return nullptr;

	uiInfo.EffectType = obj["UI_EFFECT_TYPE"];
	uiInfo.Name = obj["Name"];

	uiInfo.SizeX = obj["SizeX"];
	uiInfo.SizeY = obj["SizeY"];

	uiInfo.Alpha = obj["Alpha"];
	uiInfo.AlphaRatio = obj["AlphaRatio"];

	uiInfo.Weight = obj["Weight"];

	uiInfo.LocalX = obj["LocalX"];
	uiInfo.LocalY = obj["LocalY"];

	uiInfo.WidthRatioX = obj["WidthRatioX"];
	uiInfo.WidthRatioY = obj["WidthRatioY"];

	uiInfo.WeightOffset = obj["WeightOffset"];

	uiInfo.Restag = obj["ResTag"];

	uiInfo.Rot = obj["Rot"];
	uiInfo.LocalRot = obj["LocalRot"];

	auto color = obj["Color"];
	uiInfo.Color = { color[0], color[1], color[2] };

	if (parent == nullptr)
	{
		uiInfo.fX = obj["X"];
		uiInfo.fY = obj["Y"];
	}
	else
	{
		pUI->SetParent(parent->GetHandle());
		parent->AddChildren(pUI->GetHandle());

		uiInfo.LocalX = obj["LocalX"];
		uiInfo.LocalX = obj["LocalY"];
	}

	// 부모 기준으로 다시 계산
	pUI->CalcUICoord();

	for (const auto& child : obj["Children"])
	{
		LoadUIRecursive(child, pUI);
	}

	return pUI;
}

void CLevelUIEditor::StateView()
{

	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Target_State");
	ImGui::Separator();

	ImGui::Text("PosX  : %.2f  ", m_UIINFO.fX);
	ImGui::SameLine(150);
	ImGui::Text("PosY   : %.2f", m_UIINFO.fY);

	ImGui::Text("SizeX : %.2f  ", m_UIINFO.SizeX);
	ImGui::SameLine(150);
	ImGui::Text("SizeY  : %.2f", m_UIINFO.SizeY);

	ImGui::Text("Alpha : %.2f", m_UIINFO.Alpha);
	ImGui::SameLine(150);
	ImGui::Text("Weight : %.d", m_UIINFO.Weight);

	ImGui::Text("Name : %s", m_cName);
	m_UIINFO.Name = m_cName;
	ImGui::SameLine(150);
	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		std::optional<CHandle> parentNode = selectUI->GetParent();
		if (parentNode != std::nullopt)
		{
			Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*parentNode);
			strcpy_s(m_sParentName, parentUI->GetName());
		}
		else
			strcpy_s(m_sParentName, "None");
	}
	else
		strcpy_s(m_sParentName, "None");

	ImGui::Text("Parent : %s", m_sParentName);

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Input_State");
	ImGui::Separator();

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fx", &m_UIINFO.fX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fy", &m_UIINFO.fY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeX", &m_UIINFO.SizeX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeY", &m_UIINFO.SizeY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fAlpha", &m_UIINFO.Alpha, 0.001f, 0.0f, 1.f);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragInt("iWeight", &m_UIINFO.Weight, 1.f, 0.0f, 100);

	ImGui::SetNextItemWidth(80);
	ImGui::InputText("Name", m_cName, sizeof(m_cName));

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fColor.r", &m_UIINFO.Color.x, 0.001f, 0.0f, 1.f);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fColor.g", &m_UIINFO.Color.y, 0.001f, 0.0f, 1.f);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fColor.b", &m_UIINFO.Color.z, 0.001f, 0.0f, 1.f);

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		selectUI->SetColor(m_vColor);
	}
}

void CLevelUIEditor::LocalStateView()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
	std::optional<CHandle> parentNode = selectUI->GetParent();
	Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*parentNode);

	UI_INFO& selectInfo = selectUI->GetUIInfo();
	UI_INFO& parentInfo = parentUI->GetUIInfo();

	_float localX, localY, widthX, widthY, alphaRatio;
	int weightOffset;
	localX = selectInfo.LocalX;
	localY = selectInfo.LocalY;
	widthX = selectInfo.WidthRatioX;
	widthY = selectInfo.WidthRatioY;
	alphaRatio = selectInfo.AlphaRatio;
	weightOffset = selectInfo.WeightOffset;

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Target_LocalState");
	ImGui::Separator();

	ImGui::Text("PosX  : %.2f  ", m_UIINFO.fX);
	ImGui::SameLine(150);
	ImGui::Text("PosY   : %.2f", m_UIINFO.fY);

	ImGui::Text("SizeX : %.2f  ", m_UIINFO.SizeX);
	ImGui::SameLine(150);
	ImGui::Text("SizeY  : %.2f", m_UIINFO.SizeY);

	ImGui::Text("Alpha : %.2f", m_UIINFO.Alpha);
	ImGui::SameLine(150);
	ImGui::Text("Weight : %.d", m_UIINFO.Weight);

	ImGui::Text("Name : %s", m_cName);
	ImGui::SameLine(150);
	if (std::nullopt != Target_UI)
	{
		if (parentNode != std::nullopt)
		{
			strcpy_s(m_sParentName, parentUI->GetName());
		}
		else
			strcpy_s(m_sParentName, "None");
	}
	else
		strcpy_s(m_sParentName, "None");
	ImGui::Text("Parent : %s", m_sParentName);

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Input_State");
	ImGui::Separator();

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("LocalX", &localX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("LocalY", &localY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("WidthX", &widthX, 0.001f, 0.0f, 1.f);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("WidthY", &widthY, 0.001f, 0.0f, 1.f);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("AlphaRatio", &alphaRatio, 0.001f, 0.0f, 1.f);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragInt("WeightOffset", &weightOffset, 1.f, 0, 100);

	ImGui::SetNextItemWidth(80);
	ImGui::InputText("Name", m_cName, sizeof(m_cName));
	m_UIINFO.Name = m_cName;
	//ImGui::InputText("Name", m_cName, sizeof(m_cName));

	selectInfo.LocalX = localX;
	selectInfo.LocalY = localY;
	selectInfo.WidthRatioX = widthX;
	selectInfo.WidthRatioY = widthY;
	selectInfo.Alpha = alphaRatio;
	selectInfo.WeightOffset = weightOffset;
}

void CLevelUIEditor::DrawFileExplorer()
{

}

void CLevelUIEditor::DeleteUIRecursive(std::optional<CHandle> targetHandle)
{
	Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetHandle);

	std::vector<CHandle>  childHandles = targetUI->GetChildren();

	for (auto childHandle : childHandles)
	{
		DeleteUIRecursive(childHandle);
	}

	if (targetUI->GetParent())
	{
		Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetUI->GetParent());

		if (nullptr != parentUI)
			parentUI->DeleteChild(targetUI->GetHandle());
	}

	if (0 == childHandles.size())
	{
		selectedParent--;
		targetUI->SetPendingDestroyCascade();
		return;
	}

	selectedParent--;
	targetUI->SetPendingDestroyCascade();

	return;
}

void CLevelUIEditor::RefreshJsonFileList()
{
	g_JsonFiles.clear();

	switch (m_iEditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE) :
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/LevelUI/");
		break;
	case ETOUI(UiEditorMode::PREFAB):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/Prefabs/");
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/FlipBook/");
		break;
	}

	if (!fs::exists(g_BasePath) || !fs::is_directory(g_BasePath))
		return;

	try
	{
		for (const auto& entry : fs::directory_iterator(g_BasePath))
		{
			// 오직 일반 파일이면서 확장자가 .json인 것만 수집
			if (entry.is_regular_file() && entry.path().extension() == ".json")
			{
				JsonFileInfo info;
				info.fileName = entry.path().filename().string();
				info.fullPath = entry.path().string();
				g_JsonFiles.push_back(info);
			}
		}
	}
	catch (const std::exception& e)
	{
		// 에러 처리 예시
	}
}

void CLevelUIEditor::DrawJsonFileLoader(uint32_t EditorMode)
{
	if (!g_IsFileGridInitialized)
	{
		RefreshJsonFileList();
		g_IsFileGridInitialized = true;
	}
	std::string title = "";
	switch (EditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE):
		title = "Level Selector";
		break;
	case ETOUI(UiEditorMode::PREFAB):
		title = "PREFAB Selector";
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		title = "FLIPBOOK Selector";
		break;
	}

	ImGui::Begin(title.c_str());

	// 파일이 추가되었을 때를 대비한 새로고침 버튼
	if (ImGui::Button("Refresh"))
	{
		RefreshJsonFileList();
	}

	ImGui::Separator();

	// 스크롤 가능한 목록 영역 시작
	ImGui::BeginChild("JsonListArea", ImVec2(0, 0), true);

	if (g_JsonFiles.empty())
	{
		ImGui::TextDisabled("경로 내에 JSON 파일이 없습니다.");
	}
	else
	{
		for (const auto& file : g_JsonFiles)
		{
			// 리스트 형태로 파일명을 출력하고, 클릭 감지
			if (ImGui::Selectable(file.fileName.c_str(), false))
			{
				switch (EditorMode)
				{
				case ETOUI(UiEditorMode::ARRANGE):
					strcpy_s(m_cLevelName, sizeof(m_cLevelName), file.fileName.substr(0, file.fileName.length() - 5).c_str());
					Load();
					break;
				case ETOUI(UiEditorMode::PREFAB):
					strcpy_s(m_cPrefabName, sizeof(m_cPrefabName), file.fileName.substr(0, file.fileName.length() - 5).c_str());
					PrefabLoad();
					break;
				case ETOUI(UiEditorMode::FLIPBOOK):
					strcpy_s(m_cPrefabName, sizeof(m_cPrefabName), file.fileName.substr(0, file.fileName.length() - 5).c_str());
					PrefabLoad();
					break;
				}

				// 디버깅용 콘솔 출력
				printf("로드 대상 파일: %s\n", file.fullPath.c_str());
			}
		}
	}

	ImGui::EndChild();
	ImGui::End();
}

Engine::UPtr<CLevelUIEditor> CLevelUIEditor::Create()
{
	auto	pInstance = Engine::UPtr<CLevelUIEditor>(new CLevelUIEditor{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelUIEditor");
	}

	return pInstance;
}

void CLevelUIEditor::Free()
{
	E::CGameInstance::Get().DelPrototype("LEVEL_UIEditor");
	E::CGameInstance::Get().DelResource("LEVEL_UIEditor");
	CLevel::Free();
}
