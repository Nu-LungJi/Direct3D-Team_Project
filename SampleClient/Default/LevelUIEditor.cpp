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
#include <fstream>

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

	if (std::nullopt == Target_UI)
	{
		m_fX = clientSize.x * 0.5f;
		m_fY = clientSize.y * 0.5f;
		m_fSizeX = 100.f;
		m_fSizeY = 100.f;
		m_fAlpha = 1.f;
		m_iWeight = 0;
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

	return S_OK;
}

void CLevelUIEditor::Update(E::_float fTimeDelta)
{
	_bool bC = CGameInstance::Get().KeyPressing(DIK_C);
	_bool bV = CGameInstance::Get().KeyPressing(DIK_V);
	_bool bDelete = CGameInstance::Get().KeyDown(DIK_DELETE);

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
		selectUI->SetSize({ m_fSizeX, m_fSizeY });
		selectUI->SetAlpha(m_fAlpha);
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		selectUI->SetOrigin({ m_fX, m_fY });
		selectUI->SetSize({ m_fSizeX, m_fSizeY });
		selectUI->SetAlpha(m_fAlpha);
		selectUI->SetWeight(m_iWeight);
		selectUI->SetName(m_cName);
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
	return E_NOTIMPL;
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
			CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*m_oSelectHandle);
			_float2 size = selectUI->GetSize();
			_float2 mousePos = E::CGameInstance::Get().GetMousePos();
			
			count++;

			CTexUI::UIOBJECT_DESC Desc{};
			Desc.sObjectTag = "UI_" + std::to_string(count);
			Desc.fSizeX = m_fSizeX;
			Desc.fSizeY = m_fSizeY;
			Desc.fX = mousePos.x;
			Desc.fY = mousePos.y;
			Desc.fAlpha = m_fAlpha;
			Desc.ResTag = selectUI->Get_ResTag();
			Desc.ResWeight = count;

			E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TexUI", "Layer_UI", &Desc);
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

		_float2 mousePos = CGameInstance::Get().GetMousePos();

		m_fX = mousePos.x - m_vDragOffset.x;
		m_fY = mousePos.y - m_vDragOffset.y;

		if (std::nullopt != selectUI->GetParent())
		{
			Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*selectUI->GetParent());

			selectUI->SetLocalX(m_fX - parentUI->GetOrigin().x);
			selectUI->SetLocalY(m_fY - parentUI->GetOrigin().y);
		}
		else
			selectUI->SetOrigin({ m_fX, m_fY });
	}
}

void CLevelUIEditor::ArrangeMode()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	
	_float2 mousePos = CGameInstance::Get().GetMousePos();

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
		m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);

	ImGui::SameLine();

	if (ImGui::Button("PREFAB_MODE"))
		m_iEditorMode = ETOUI(UiEditorMode::PREFAB);

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Target_State");
	ImGui::Separator();

	ImGui::Text("PosX  : %.2f  ", m_fX);
	ImGui::SameLine(150);
	ImGui::Text("PosY   : %.2f", m_fY);

	ImGui::Text("SizeX : %.2f  ", m_fSizeX);
	ImGui::SameLine(150);
	ImGui::Text("SizeY  : %.2f", m_fSizeY);

	ImGui::Text("Alpha : %.2f", m_fAlpha);
	ImGui::SameLine(150);
	ImGui::Text("Weight : %.d", m_iWeight);

	ImGui::Text("Name : %s", m_cName);

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Input_State");
	ImGui::Separator();

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fx", &m_fX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fy", &m_fY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeX", &m_fSizeX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeY", &m_fSizeY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fAlpha", &m_fAlpha, 0.001f, 0.0f, 1.f);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragInt("iWeight", &m_iWeight, 1.f, 0.0f, 100);

	ImGui::SetNextItemWidth(80);
	ImGui::InputText("Name", m_cName, sizeof(m_cName));

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
					CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*m_oSelectHandle);
					selectUI->SetPendingDestroyCascade();

					m_oSelectHandle = std::nullopt;
				}

				CTexUI::UIOBJECT_DESC Desc{};
				Desc.sObjectTag = "Select_Image";
				Desc.fSizeX = m_fSizeX;
				Desc.fSizeY = m_fSizeY;
				Desc.fX = g_iWinSizeX * 0.5f;
				Desc.fY = g_iWinSizeY * 0.5f;
				Desc.fAlpha = m_fAlpha;
				Desc.ResTag = m_vResTag[i];

				m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TexUI","Layer_UI_Texture", &Desc);
				CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*m_oSelectHandle);
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
		m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);

	ImGui::SameLine();

	if (ImGui::Button("PREFAB_MODE"))
		m_iEditorMode = ETOUI(UiEditorMode::PREFAB);

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		
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

					targetUI->SetLocalX(m_fX - parentUI->GetOrigin().x);
					targetUI->SetLocalY(m_fY - parentUI->GetOrigin().y);
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
					CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*m_oSelectHandle);
					selectUI->SetPendingDestroyCascade();

					m_oSelectHandle = std::nullopt;
				}

				Engine::CUIObject::UIOBJECT_DESC Desc{};
				Desc.sObjectTag = "Select_Image";
				Desc.fSizeX = m_fSizeX;
				Desc.fSizeY = m_fSizeY;
				Desc.fX = g_iWinSizeX * 0.5f;
				Desc.fY = g_iWinSizeY * 0.5f;
				Desc.fAlpha = m_fAlpha;
				Desc.ResTag = m_vResTag[i];

				m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TexUI", "Layer_UI_Texture", &Desc);
				CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*m_oSelectHandle);
				selectUI->SetMouseTracking(true);
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
		selectUI->GetOrigin(); // _float2 위치
		selectUI->GetSize(); // _float2 사이즈

		_float2 origin = selectUI->GetOrigin(); 
		_float2 size = selectUI->GetSize();

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
			uint32_t curWeight = selectUI->GetWeight();
			if (curWeight > maxWeight)
			{
				maxWeight = curWeight;
				Target_UI = ui;

				m_vDragOffset = { CGameInstance::Get().GetMousePos().x - selectUI->GetOrigin().x,
					CGameInstance::Get().GetMousePos().y - selectUI->GetOrigin().y };
			}
		}
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		m_fX = selectUI->GetOrigin().x;
		m_fY = selectUI->GetOrigin().y;
		m_fSizeX = selectUI->GetSize().x;
		m_fSizeY = selectUI->GetSize().y;
		m_fAlpha = selectUI->GetAlpha();
		m_iWeight = selectUI->GetWeight();
		strcpy_s(m_cName, sizeof(m_cName), selectUI->GetName());
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
		selectUI->GetOrigin(); // _float2 위치
		selectUI->GetSize(); // _float2 사이즈

		_float2 origin = selectUI->GetOrigin();
		_float2 size = selectUI->GetSize();

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
			uint32_t curWeight = selectUI->GetWeight();
			if (curWeight > maxWeight)
			{
				maxWeight = curWeight;
				Target_UI = ui;

				m_vDragOffset = { CGameInstance::Get().GetMousePos().x - selectUI->GetOrigin().x,
					CGameInstance::Get().GetMousePos().y - selectUI->GetOrigin().y };
			}
		}
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		m_fX = selectUI->GetOrigin().x;
		m_fY = selectUI->GetOrigin().y;
		m_fSizeX = selectUI->GetSize().x;
		m_fSizeY = selectUI->GetSize().y;
		m_fAlpha = selectUI->GetAlpha();
		m_iWeight = selectUI->GetWeight();
		strcpy_s(m_cName, sizeof(m_cName), selectUI->GetName());
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
		CTexUI* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(handle);

		if (pUI == nullptr)
			continue;

		if (pUI->GetParent().has_value())
			continue;

		nlohmann::ordered_json obj;
		SaveUIRecursive(pUI, obj);

		root["UI"].push_back(obj);
	}

	char path[256] = "./Resources/SampleClient/UIData/Prefabs/";
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
	char path[256] = "./Resources/SampleClient/UIData/Prefabs/";
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

void CLevelUIEditor::SaveUIRecursive(CTexUI* pUI, nlohmann::ordered_json& obj)
{
	obj["Name"] = pUI->GetName();

	obj["X"] = pUI->GetWorldPos().x;
	obj["Y"] = pUI->GetWorldPos().y;

	obj["LocalX"] = pUI->GetLocalX();
	obj["LocalY"] = pUI->GetLocalY();

	obj["SizeX"] = pUI->GetSize().x;
	obj["SizeY"] = pUI->GetSize().y;

	obj["WidthRatioX"] = pUI->GetWidthRatioX();
	obj["WidthRatioY"] = pUI->GetWidthRatioY();

	obj["Alpha"] = pUI->GetAlpha();
	obj["AlphaRatio"] = pUI->GetAlphaRatio();

	obj["Weight"] = pUI->GetWeight();
	obj["WeightOffset"] = pUI->GetWeightOffset();

	obj["ResTag"] = pUI->Get_ResTag();

	obj["UIType"] = pUI->GetUIType();

	obj["Children"] = nlohmann::ordered_json::array();

	

	for (CHandle childHandle : pUI->GetChildren())
	{
		CTexUI* pChild = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(childHandle);

		if (pChild == nullptr)
			continue;

		nlohmann::ordered_json childObj;
		SaveUIRecursive(pChild, childObj);

		obj["Children"].push_back(childObj);
	}
}

CTexUI* CLevelUIEditor::LoadUIRecursive(const nlohmann::ordered_json& obj, CTexUI* parent)
{
	uint32_t uiType = obj["UIType"];

	CTexUI* pUI = nullptr;

	count++;

	CTexUI::UIOBJECT_DESC Desc{};
	Desc.sObjectTag = obj["Name"];

	std::optional<CHandle> uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TexUI", "Layer_UI", &Desc);
	pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*uiHandle);

	if (pUI == nullptr)
		return nullptr;

	pUI->SetName(obj["Name"]);

	pUI->SetSize(
		{
			obj["SizeX"],
			obj["SizeY"]
		});

	pUI->SetAlpha(obj["Alpha"]);

	pUI->SetWeight(obj["Weight"]);

	pUI->SetLocalX(obj["LocalX"]);
	pUI->SetLocalY(obj["LocalY"]);

	pUI->SetWidthRatioX(obj["WidthRatioX"]);
	pUI->SetWidthRatioY(obj["WidthRatioY"]);

	pUI->SetAlphaRatio(obj["AlphaRatio"]);
	pUI->SetWeightOffset(obj["WeightOffset"]);

	pUI->Set_ResTag(obj["ResTag"]);

	if (parent == nullptr)
	{
		pUI->SetWorldPos(
			{
				obj["X"],
				obj["Y"]
			});
	}
	else
	{
		pUI->SetParent(parent->GetHandle());
		parent->AddChildren(pUI->GetHandle());

		pUI->SetLocalPos(
			{
				obj["LocalX"],
				obj["LocalY"]
			});

		// 부모 기준으로 다시 계산
		//pUI->CalcUICoord();
	}

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

	ImGui::Text("PosX  : %.2f  ", m_fX);
	ImGui::SameLine(150);
	ImGui::Text("PosY   : %.2f", m_fY);

	ImGui::Text("SizeX : %.2f  ", m_fSizeX);
	ImGui::SameLine(150);
	ImGui::Text("SizeY  : %.2f", m_fSizeY);

	ImGui::Text("Alpha : %.2f", m_fAlpha);
	ImGui::SameLine(150);
	ImGui::Text("Weight : %.d", m_iWeight);

	ImGui::Text("Name : %s", m_cName);
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
	ImGui::DragFloat("fx", &m_fX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fy", &m_fY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeX", &m_fSizeX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeY", &m_fSizeY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fAlpha", &m_fAlpha, 0.001f, 0.0f, 1.f);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragInt("iWeight", &m_iWeight, 1.f, 0.0f, 100);

	ImGui::SetNextItemWidth(80);
	ImGui::InputText("Name", m_cName, sizeof(m_cName));
}

void CLevelUIEditor::LocalStateView()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
	std::optional<CHandle> parentNode = selectUI->GetParent();
	Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*parentNode);

	_float localX, localY, widthX, widthY, alphaRatio;
	int weightOffset;
	localX = selectUI->GetLocalX();
	localY = selectUI->GetLocalY();
	widthX = selectUI->GetWidthRatioX();
	widthY = selectUI->GetWidthRatioY();
	alphaRatio = selectUI->GetAlphaRatio();
	weightOffset = selectUI->GetWeightOffset();

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Target_LocalState");
	ImGui::Separator();

	ImGui::Text("PosX  : %.2f  ", m_fX);
	ImGui::SameLine(150);
	ImGui::Text("PosY   : %.2f", m_fY);

	ImGui::Text("SizeX : %.2f  ", m_fSizeX);
	ImGui::SameLine(150);
	ImGui::Text("SizeY  : %.2f", m_fSizeY);

	ImGui::Text("Alpha : %.2f", m_fAlpha);
	ImGui::SameLine(150);
	ImGui::Text("Weight : %.d", m_iWeight);

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

	selectUI->SetLocalX(localX);
	selectUI->SetLocalY(localY);
	selectUI->SetWidthRatioX(widthX);
	selectUI->SetWidthRatioY(widthY);
	selectUI->SetAlphaRatio(alphaRatio);
	selectUI->SetWeightOffset(weightOffset);
}

void CLevelUIEditor::DeleteUIRecursive(std::optional<CHandle> targetHandle)
{
	Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetHandle);

	std::vector<CHandle>  childHandles = targetUI->GetChildren();

	for (auto childHandle : childHandles)
	{
		DeleteUIRecursive(childHandle);
	}

	if (0 == childHandles.size())
	{
		selectedParent--;
		targetUI->SetPendingDestroyCascade();
		return;
	}

	if (targetUI->GetParent())
	{
		Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetUI->GetParent());

		if (parentUI)
			parentUI->DeleteChild(targetUI->GetHandle());
	}
	selectedParent--;
	targetUI->SetPendingDestroyCascade();

	return;
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
