#include "pch.h"
#include "LevelUIEditor.h"
#include "GameInstance.h"
#include "LevelLoading.h"

#include "FlyCamera.h"
#include "UiCamera.h"
#include "ResCBuffer.h"
#include "ResTexture2D.h"
#include "CTexUI.h"
#include <fstream>

NS_USING(Client)

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
			CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*m_oSelectHandle);
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
			CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*m_oSelectHandle);
			selectUI->SetPendingDestroyCascade();

			m_oSelectHandle = std::nullopt;
		}

		if (std::nullopt != Target_UI)
		{
			CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*Target_UI);
			selectUI->SetPendingDestroyCascade();

			Target_UI = std::nullopt;
		}
	}

	if (std::nullopt != m_oSelectHandle)
	{
		CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*m_oSelectHandle);
		selectUI->SetSize({ m_fSizeX, m_fSizeY });
		selectUI->SetAlpha(m_fAlpha);
	}

	if (std::nullopt != Target_UI)
	{
		CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*Target_UI);
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
			Desc.sObjectTag = "UI_" + count;
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

	if(MouseLB)
		Picking();

	if (MouseLBPressing && std::nullopt != Target_UI)
	{
		CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*Target_UI);

		_float2 mousePos = CGameInstance::Get().GetMousePos();

		m_fX = mousePos.x - m_vDragOffset.x;
		m_fY = mousePos.y - m_vDragOffset.y;

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

				m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TexUI", "Layer_UI_Texture", &Desc);
				CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*m_oSelectHandle);
				selectUI->SetMouseTracking(true);
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}
}

void CLevelUIEditor::Picking()
{
	_float2 mousePos = CGameInstance::Get().GetMousePos();

	std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	Target_UI = std::nullopt;

	uint32_t maxWeight = 0;

	for (auto ui : uiHandles)
	{
		CTexUI* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(ui);

		if (nullptr == checkUI)
			continue;

		CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(ui);
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
		CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(*Target_UI);
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

	nlohmann::json root;

	root["LevelName"] = m_cLevelName;

	for (auto ui : uiHandles)
	{
		CTexUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(ui);

		nlohmann::json obj;

		obj["Name"] = selectUI->GetName();
		obj["X"] = selectUI->GetOrigin().x;
		obj["Y"] = selectUI->GetOrigin().y;
		obj["SizeX"] = selectUI->GetSize().x;
		obj["SizeY"] = selectUI->GetSize().y;
		obj["Alpha"] = selectUI->GetAlpha();
		obj["Weight"] = selectUI->GetWeight();
		obj["ResTag"] = selectUI->Get_ResTag();

		root["UI"].push_back(obj);
	}

	char path[256] = "./Resources/SampleClient/UIData/";
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
}

void CLevelUIEditor::Load()
{
	E::CGameInstance::Get().DelGameObjectLayer("Layer_UI");

	char path[256] = "./Resources/SampleClient/UIData/";
	strcat_s(path, sizeof(path), m_cLevelName);

	char ext[] = ".json";
	strcat_s(path, sizeof(path), ext);

	std::ifstream file(path);

	if (!file.is_open())
	{
		MSG_BOX("파일 열기 실패");
		return;
	}

	nlohmann::json root;
	file >> root;

	strcpy_s(m_cLevelName,
		sizeof(m_cLevelName),
		root["LevelName"].get<std::string>().c_str());

	for (const auto& obj : root["UI"])
	{
		std::string name = obj["Name"];

		float x = obj["X"];
		float y = obj["Y"];
		float sizeX = obj["SizeX"];
		float sizeY = obj["SizeY"];
		float alpha = obj["Alpha"];
		int weight = obj["Weight"];
		std::string resTag = obj["ResTag"];

		// UI 생성

		CTexUI::UIOBJECT_DESC Desc{};

		Desc.sObjectTag = name;
		Desc.fSizeX = sizeX;
		Desc.fSizeY = sizeY;
		Desc.fX = x;
		Desc.fY = y;
		Desc.fAlpha = alpha;
		Desc.ResTag = resTag;
		Desc.ResWeight = weight;

		E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TexUI", "Layer_UI", &Desc);
	}

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
