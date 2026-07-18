#include "pch.h"
#include "GameInstance.h"
#include "UIManager.h"
#include "TextureUI.h"
#include "EffectUI.h"
#include "TextBox.h"
#include "Button.h"
#include <fstream>
#include "LevelLogo.h"
#include "LevelLoading.h"
#include "SpellMeter.h"
#include "HPBar.h"

NS_USING(Client)

UIManager::~UIManager()
{

}

void UIManager::Update()
{
}

void UIManager::Initialize(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
	m_pDevice = pDevice;
	m_pContext = pContext;
}

void UIManager::InitializeActions()
{
	// ==========================================
	// 1. 사이즈 업
	// ==========================================
	m_EventMap["ScaleUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 1.1f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			});
	};
	m_vEventNames.push_back("ScaleUp");

	// ==========================================
	// 2. 사이즈 축소
	// ==========================================
	m_EventMap["ScaleDown"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 1.0f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			});
	};
	m_vEventNames.push_back("ScaleDown");

	// ==========================================
	// 3. 페이드 인
	// ==========================================
	m_EventMap["FadeIn"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		//pCaller->GetAlpha()
		pTween->PlayTween(0.f, 1.0f, 0.5f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});
	};
	m_vEventNames.push_back("FadeIn");

	m_EventMap["LocalFadeIn"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		pTween->PlayTween(pCaller->GetAlphaRatio(), 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			});
	};
	m_vEventNames.push_back("LocalFadeIn");

	// ==========================================
	// 4. 페이드 아웃
	// ==========================================
	m_EventMap["FadeOut"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetAlpha(), 0.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			}, 
			[handle]() {
				if (auto pObj = GetSafeUI(handle)) pObj->SetActive(false);
			});
	};
	m_vEventNames.push_back("FadeOut");

	m_EventMap["LocalFadeOut"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetAlphaRatio(), 0.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			},
			[handle]() {
				if (auto pObj = GetSafeUI(handle)) pObj->SetActive(false);
			});
	};
	m_vEventNames.push_back("LocalFadeOut");

	// ==========================================
	// 5. 페이드 인 & 아웃 (FadInOut -> FadeInOut 권장)
	// ==========================================
	m_EventMap["FadInOut"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(0.f, 1.f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			},
			[handle]() {
				if (auto pObj = GetSafeUI(handle))
				{
					if (auto pNextTween = pObj->GetTweenCom())
					{
						pNextTween->PlayTween(1.f, 0.f, 0.3f,
							[handle](float currentValue) {
								if (auto pObj2 = GetSafeUI(handle)) pObj2->SetAlphaRatio(currentValue);
							},
							[handle]() {
								if (auto pObj2 = GetSafeUI(handle)) pObj2->SetActive(false);
							});
					}
				}
			});
	};
	m_vEventNames.push_back("FadInOut");

	// ==========================================
	// 6. 스케일 업 & 다운
	// ==========================================
	m_EventMap["ScaleUpDown"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 1.2f, 0.08f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			},
			[handle]() {
				if (auto pObj = GetSafeUI(handle)) 
				{
					if (auto pNextTween = pObj->GetTweenCom()) 
					{
						pNextTween->PlayTween(1.2f, 1.1f, 0.08f,
							[handle](float currentValue) {
								if (auto pObj2 = GetSafeUI(handle)) {
									pObj2->SetScaleRatio(currentValue);
									pObj2->CalcUICoord();
								}
							});
					}
				}
			});
	};
	m_vEventNames.push_back("ScaleUpDown");

	// ==========================================
	// 위치 업
	// ==========================================
	m_EventMap["PosUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		pCaller->SetInputLcok(true);

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 30.f, 0.4f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ pObj->GetPos().x, originalPos.y - currentValue });
					pObj->CalcUICoord();
				}
			}, [handle]() {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetInputLcok(false);
				}
			});

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("PosUp");

	m_EventMap["LocalPosUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetLocalPos();
		pTween->PlayTween(0, 30.f, 0.4f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ pObj->GetLocalPos().x, originalPos.y - currentValue });
					pObj->CalcUICoord();
				}
			});

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			});
	};
	m_vEventNames.push_back("LocalPosUp");

	// ==========================================
	// 오른쪽
	// ==========================================
	m_EventMap["PosRight"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 30.f, 0.4f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ originalPos.x + currentValue, originalPos.y });
					pObj->CalcUICoord();
				}
			});

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("PosRight");

	// ==========================================
	// 바운스
	// ==========================================
	m_EventMap["Bounce"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		pCaller->SetActive(true);
		pCaller->SetInputLcok(true);

		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 150.f, 1.f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ originalPos.x, originalPos.y + currentValue });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutBounce);

		pTween->PlayTween(0, 80.f, 1.f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					_float2 pos = pObj->GetPos();
					pObj->SetPos({ originalPos.x + currentValue, pos.y });
					pObj->CalcUICoord();
				}
			}, [handle]() {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetInputLcok(false);
				}
			}, EEaseType::EaseOutQuad);

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("Bounce");


	// ==========================================
	// 탄성
	// ==========================================
	m_EventMap["Elastic"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 100.f, 1.f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ pObj->GetPos().x, originalPos.y + currentValue });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutElastic);

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("Elastic");

	// ==========================================
	// 오버슛
	// ==========================================
	m_EventMap["OverShoot"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 50.f, 0.5f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ originalPos.x + currentValue, originalPos.y });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutBack);

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("OverShoot");

	// ==========================================
	// 둥둥
	// ==========================================
	m_EventMap["Floating"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		float startY = pCaller->GetUIInfo().fY;
		float endY = startY + 15.0f;

		pTween->PlayTween(startY, endY, 0.7f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->GetUIInfo().fY = currentValue;
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::Floating, 0.0f, true);
	};
	m_vEventNames.push_back("Floating");

	// ==========================================
	// 순차
	// ==========================================
	int maxIterations = 10;
	for (int i = 1; i <= maxIterations; ++i)
	{
		float delay = i * 0.3f;

		char szName[32];
		snprintf(szName, sizeof(szName), "PosUp%.1f", delay);
		std::string eventName = szName;

		m_EventMap[eventName] = [delay](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float2 originalPos = pCaller->GetPos();

			pTween->PlayTween(0.f, 30.f, 0.4f,
				[handle, originalPos](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetPos({ pObj->GetPos().x, originalPos.y - currentValue });
						pObj->CalcUICoord();
					}
				}, [handle]() {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetInputLcok(false);
					}
				}, EEaseType::Linear, delay, false);

			pTween->PlayTween(0.f, 1.0f, 0.3f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
				}, nullptr, EEaseType::Linear, delay, false);
		};
		m_vEventNames.push_back(eventName);
	}

	// ==========================================
	// 펄스
	// ==========================================
	m_EventMap["LockOnEffect"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		float startSizeX = pCaller->GetUIInfo().SizeX;
		float targetSizeX = startSizeX * 2.0f;

		pTween->PlayTween(startSizeX, targetSizeX, 0.6f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->GetUIInfo().SizeX = currentValue;
					pObj->GetUIInfo().SizeY = currentValue; 
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad, 0.0f, true); 

		pTween->PlayTween(1.0f, 0.0f, 0.6f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetAlphaRatio(currentValue);
				}
			}, nullptr, EEaseType::EaseOutQuad, 0.0f, true);
	};
	m_vEventNames.push_back("LockOnEffect");
}

void UIManager::InitializeFunc()
{
	m_FuncMap["Create"] = [](std::string name)
	{
		GET_SINGLE(UIManager)->LoadPrefab(name);
	};
	m_vFuncNames.push_back("Create");

	m_FuncMap["SceneChange"] = [this](std::string name)
	{
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LOGO));
	};
	m_vFuncNames.push_back("SceneChange");
}

void UIManager::UpdateRootUIHandles()
{
	std::vector<Engine::CUIObject*> uiList;

	if (nullptr == CGameInstance::Get().GetGameObjectLayer("Layer_UI"))
		return;

	rootUIHandles.clear();

	const std::vector<CHandle>* uiHandles = CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	for (auto ui : *uiHandles)
	{
		Engine::CUIObject* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);

		if (checkUI != nullptr)
		{
			if (std::nullopt == checkUI->GetParent())
			{
				rootUIHandles.push_back(ui);
			}
		}
	}
}

std::function<void(CUIObject* pCaller)> UIManager::GetAction(const std::string& actionName)
{
	auto iter = m_EventMap.find(actionName);
	if (iter != m_EventMap.end())
		return iter->second;

	MSG_BOX("[UI Error] Action not found: ");
	return [](CUIObject*) {};
}

std::function<void(std::string text)> UIManager::GetFunc(const std::string& funcName)
{
	auto iter = m_FuncMap.find(funcName);
	if (iter != m_FuncMap.end())
		return iter->second;

	MSG_BOX("[UI Error] Func not found: ");
	return [](std::string text) {};
}

std::optional<CHandle> UIManager::RootUIPicking()
{
	std::optional<CHandle> targetHandle = std::nullopt;
	for (auto uiHandle : rootUIHandles)
	{
		if (nullptr == E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(uiHandle))
			continue;

		CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(uiHandle);
		const UI_INFO& pInfo = pUI->GetUIInfo();

		if (PtInRect(pInfo))
		{
			if (std::nullopt == targetHandle)
				targetHandle = uiHandle;
			else
			{
				if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*targetHandle))
				{
					CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*targetHandle);
					const UI_INFO& targetInfo = targetUI->GetUIInfo();

					if (pInfo.Weight > targetInfo.Weight)
						targetHandle = uiHandle;
				}
			}
		}
	}

	return targetHandle;
}

_bool UIManager::PtInRect(const UI_INFO& selectInfo)
{
	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

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
		return true;
	}

	return false;
}

std::optional<CHandle> UIManager::LoadPrefab(std::string name, std::string g_BasePath)
{
	char path[256] = "";
	strcpy_s(path, sizeof(path), g_BasePath.c_str());
	strcat_s(path, sizeof(path), name.c_str());
	strcat_s(path, sizeof(path), ".json");

	std::ifstream file(path);

	if (!file.is_open())
	{
		MSG_BOX("파일 열기 실패");
		return std::nullopt;
	}

	nlohmann::ordered_json root;
	file >> root;
	file.close();

	for (const auto& obj : root["UI"])
	{
		LoadUIRecursive(obj, nullptr);
	}

	return m_rootHandle;
}

E::CUIObject* UIManager::LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent)
{
	int uiType = obj["UiType"];

	E::CUIObject* pUI = nullptr;

	E::CUIObject::UIOBJECT_DESC Desc{};
	std::optional<CHandle> uiHandle = std::nullopt;

	Desc.sObjectTag = obj["Name"];

	int EffectType = obj["UI_EFFECT_TYPE"];

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
			flipInfo.cellsize = obj["CellSize"];
			flipInfo.TotalFrame = obj["TotalFrame"];
			flipInfo.Padding = obj["Padding"];
			flipInfo.Duration = obj["Duration"];
		}

		if (EffectType == ETOUI(UI_EFFECT_TYPE::HOVER))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectHovered(uiHandle);
			}
		}
		else if (EffectType == ETOUI(UI_EFFECT_TYPE::CLICK))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectClicked(uiHandle);
			}
		}
		break;
	case ETOUI(UI_TYPE::TEXT):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextBox", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*uiHandle);
		{
			TEXT_INFO& textInfo = static_cast<CTextBox*>(pUI)->GetTextInfo();
			//textInfo.Text = obj["Text"];
		}
		break;
	case ETOUI(UI_TYPE::BUTTON):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_Button", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CButton>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::SPELLMETER):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_SpellMeter", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CSpellMeter>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::HPBAR):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_HPBar", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CHPBar>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::HPFILL):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_HPBar", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CHPBar>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::HPFILL));
		break;
	case ETOUI(UI_TYPE::LEFTHPFILL):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_HPBar", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CHPBar>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::LEFTHPFILL));
		break;
	default:
		break;
	}

	if (pUI == nullptr)
		return nullptr;

	if (parent == nullptr)
	{
		if (obj.contains("ScaleRatio"))
			pUI->SetScaleRatio(obj["ScaleRatio"]);
	}
		

	UI_INFO& uiInfo = static_cast<CUIObject*>(pUI)->GetUIInfo();

	uiInfo.EffectType = obj["UI_EFFECT_TYPE"];
	uiInfo.Name = obj["Name"];

	uiInfo.SizeX = obj["SizeX"];
	uiInfo.SizeY = obj["SizeY"];

	uiInfo.Alpha = obj["Alpha"];
	uiInfo.AlphaRatio = obj["AlphaRatio"];

	uiInfo.Weight = obj["Weight"];
	uiInfo.WeightOffset = obj["WeightOffset"];

	uiInfo.LocalX = obj["LocalX"];
	uiInfo.LocalY = obj["LocalY"];

	uiInfo.WidthRatioX = obj["WidthRatioX"];
	uiInfo.WidthRatioY = obj["WidthRatioY"];

	uiInfo.Restag = obj["ResTag"];

	uiInfo.Rot = obj["Rot"];
	uiInfo.LocalRot = obj["LocalRot"];

	auto color = obj["Color"];
	uiInfo.Color = { color[0], color[1], color[2] };

	UI_EVENT& eventInfo = pUI->GetUIEvent();

	eventInfo.ClickFunc = obj.value("ClickFunc", "");
	eventInfo.ClickAction = obj.value("ClickAction", "");
	eventInfo.EnterAction = obj.value("EnterAction", "");
	eventInfo.ExitAction = obj.value("ExitAction", "");
	eventInfo.AppearAction = obj.value("AppearAction", "");
	eventInfo.DisappearAction = obj.value("DisappearAction", "");

	auto bindAction = [](const std::string& actionStr, std::function<void(CUIObject*)>& targetFunc) {
		if (!actionStr.empty() && actionStr != "None") {
			targetFunc = GET_SINGLE(UIManager)->GetAction(actionStr);
		}
	};

	bindAction(eventInfo.ClickAction, pUI->OnClicked);
	bindAction(eventInfo.EnterAction, pUI->OnHoverEnter);
	bindAction(eventInfo.ExitAction, pUI->OnHoverExit);
	bindAction(eventInfo.AppearAction, pUI->Appear);
	bindAction(eventInfo.DisappearAction, pUI->Disappear);

	if(!eventInfo.ClickFunc.empty() && eventInfo.ClickFunc != "None")
		pUI->OnClickedAction = GET_SINGLE(UIManager)->GetFunc(eventInfo.ClickFunc);


	if (parent == nullptr)
	{
		m_rootHandle = uiHandle;

		uiInfo.fX = obj["X"];
		uiInfo.fY = obj["Y"];
	}
	else
	{
		pUI->SetParent(parent->GetHandle());
		parent->AddChildren(pUI->GetHandle());

		uiInfo.LocalX = obj["LocalX"];
		uiInfo.LocalY = obj["LocalY"];
	}

	// 부모 기준으로 다시 계산
	pUI->CalcUICoord();

	for (const auto& child : obj["Children"])
	{
		LoadUIRecursive(child, pUI);
	}

	return pUI;
}

void UIManager::DeleteUIRecursive(std::optional<CHandle> targetHandle)
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

	targetUI->SetPendingDestroyCascade();

	return;
}
