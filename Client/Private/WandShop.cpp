#include "pch.h"
#include "WandShop.h"

#include "GeneralButton.h"
#include "GameInstance.h"
#include "Player.h"
#include "Player_Weapon.h"
#include "TextBox.h"
#include "TextureUI.h"
#include "TweenComponent.h"
#include "UIManager.h"

NS_USING(Client)

namespace
{
	constexpr std::array<std::string_view, 4> PAGE_PREFABS{
		"ShopWand1", "ShopWandCompleteShape", "ShopWand3", "ShopWand4"
	};
	constexpr _float PAGE_FADE_IN_DURATION = 0.18f;
	constexpr _float SHOP_OPEN_FADE_IN_DURATION = 0.3f;
	constexpr int WAND_SHOP_MIN_ROOT_WEIGHT = 800;
	constexpr _float PURCHASE_HOLD_DURATION = 1.2f;
	constexpr int PURCHASE_BUTTON_WEIGHT = 900;
	constexpr int PURCHASE_CHILD_WEIGHT = PURCHASE_BUTTON_WEIGHT + 1;
}

_bool CWandShop::IsPagePrefab(std::string_view prefabName)
{
	return std::ranges::find(PAGE_PREFABS, prefabName) != PAGE_PREFABS.end();
}

_bool CWandShop::IsCommonRootName(std::string_view objectName)
{
	static constexpr std::array<std::string_view, 14> commonNames{
		"menuBG1", "menuBG2", "menuBG3", "menuBG4",
		"BlackBG", "DividerUp", "Title", "WandImage",
		"BuySellL", "BuySellR", "GoldLeafL", "GoldLeafR", "L1", "R1"
	};
	return std::ranges::find(commonNames, objectName) != commonNames.end();
}

void CWandShop::ClassifyRoots(const std::vector<CHandle>& roots)
{
	m_CommonRoots.clear();
	m_PageRoots.clear();
	for (const CHandle handle : roots)
	{
		auto* ui = GetSafeUI(handle);
		if (!ui)
			continue;
		if (IsCommonRootName(ui->GetName()))
			m_CommonRoots.push_back(handle);
		else
			m_PageRoots.push_back(handle);
	}
}

void CWandShop::ApplyRootWeightOffset(
	const std::vector<CHandle>& roots,
	_bool initializeOffset)
{
	if (initializeOffset)
	{
		int minimumWeight = INT_MAX;
		for (const CHandle handle : roots)
		{
			if (auto* ui = GetSafeUI(handle))
				minimumWeight = std::min(minimumWeight, ui->GetUIInfo().Weight);
		}
		m_iRootWeightOffset = minimumWeight == INT_MAX ? 0 :
			WAND_SHOP_MIN_ROOT_WEIGHT - minimumWeight;
	}

	for (const CHandle handle : roots)
	{
		if (auto* ui = GetSafeUI(handle))
			ui->GetUIInfo().Weight += m_iRootWeightOffset;
	}
}

void CWandShop::RegisterLoadedPage(const std::string& prefabName,
	const std::vector<CHandle>& roots)
{
	const auto iter = std::ranges::find(PAGE_PREFABS,
		std::string_view(prefabName));
	if (iter == PAGE_PREFABS.end())
		return;
	ApplyRootWeightOffset(roots, true);
	m_iCurrentPage = static_cast<uint32_t>(
		std::distance(PAGE_PREFABS.begin(), iter));
	ClassifyRoots(roots);
	CGeneralButton::SetCurrentWandShopPage(m_iCurrentPage);
	CGeneralButton::RefreshWandShopCommonUI(m_iCurrentPage);
	// 최초 상점 로드에서는 공통 프레임과 1페이지 내용을 함께 나타낸다.
	PlayPageFadeIn(roots, SHOP_OPEN_FADE_IN_DURATION);
}

_bool CWandShop::IsOpen() const
{
	return std::ranges::any_of(m_CommonRoots,
		[](CHandle handle) { return GetSafeUI(handle) != nullptr; });
}

void CWandShop::CreatePurchasePrompt()
{
	if (!IsOpen())
		return;
	if (std::ranges::any_of(m_PurchasePromptRoots,
		[](CHandle handle) { return GetSafeUI(handle) != nullptr; }))
	{
		return;
	}

	m_PurchasePromptRoots.clear();
	m_PurchaseGauge.reset();
	m_PurchaseCoinText.reset();
	m_iDisplayedCoinCount = UINT32_MAX;
	m_fPurchaseHoldProgress = 0.f;

	auto promptRoots = GET_SINGLE(UIManager)->LoadPrefab(
		"FillButton",
		"./Resources/SampleClient/UIData/RTT/");
	if (promptRoots.empty())
		return;

	CUIObject* button = nullptr;
	CTextureUI* gauge = nullptr;
	CTextBox* coinText = nullptr;
	for (const CHandle rootHandle : promptRoots)
	{
		auto* root = GetSafeUI(rootHandle);
		if (!root)
			continue;
		root->SetInputLcok(true);
		root->GetUIInfo().Weight = PURCHASE_BUTTON_WEIGHT;
		if (std::string_view(root->GetName()) == "Button")
			button = root;

		for (const CHandle childHandle : root->GetChildren())
		{
			auto* child = GetSafeUI(childHandle);
			if (!child)
				continue;
			child->GetUIInfo().Weight = PURCHASE_CHILD_WEIGHT;
			child->SetInputLcok(true);
			if (std::string_view(root->GetName()) == "CoinImage" &&
				(std::string_view(child->GetName()) == "CoinText" ||
				 std::string_view(child->GetName()) == "Coin"))
			{
				coinText = E::CGameInstance::Get().
					GetGameObjectByHandleT<CTextBox>(childHandle);
				if (coinText)
					m_PurchaseCoinText = childHandle;
			}
			if (std::string_view(child->GetName()) == "Text")
			{
				continue;
			}
			if (std::string_view(child->GetName()) != "BTFrame")
				continue;
			gauge = E::CGameInstance::Get().
				GetGameObjectByHandleT<CTextureUI>(childHandle);
			if (gauge)
			{
				m_PurchaseGauge = childHandle;
				gauge->SetPathProgressMode(true);
				gauge->SetPathProgressType(2u);
				gauge->SetPathProgress(0.f);
				gauge->SetColor({ 1.f, 1.f, 1.f });
				gauge->SetInputLcok(true);
				gauge->CalcUICoord();
			}
		}
	}

	if (!button || !gauge)
	{
		for (const CHandle rootHandle : promptRoots)
		{
			if (GetSafeUI(rootHandle))
				GET_SINGLE(UIManager)->DeleteUIRecursive(rootHandle);
		}
		m_PurchaseGauge.reset();
		return;
	}
	m_PurchasePromptRoots = std::move(promptRoots);
	SetPurchasePromptVisible(m_iCurrentPage == 0u);
	if (m_iCurrentPage == 0u)
		PlayPageFadeIn(
			m_PurchasePromptRoots, SHOP_OPEN_FADE_IN_DURATION);
	RefreshPurchaseCoinText();
}

void CWandShop::Update(UIManager& manager, _float fTimeDelta)
{
	if (!IsOpen())
		return;
	if (m_iCurrentPage != 0u)
	{
		m_fPurchaseHoldProgress = 0.f;
		if (m_PurchaseGauge)
		{
			if (auto* gauge = E::CGameInstance::Get().
				GetGameObjectByHandleT<CTextureUI>(*m_PurchaseGauge))
			{
				gauge->SetPathProgress(0.f);
			}
		}
		return;
	}
	RefreshPurchaseCoinText();
	if (!m_PurchaseGauge)
		return;
	auto* gauge = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(*m_PurchaseGauge);
	if (!gauge)
	{
		m_PurchaseGauge.reset();
		return;
	}

	if (E::CGameInstance::Get().KeyPressing(DIK_E))
	{
		const _float safeDelta = std::isfinite(fTimeDelta) ?
			std::max(0.f, fTimeDelta) : 0.f;
		m_fPurchaseHoldProgress = std::min(
			1.f,
			m_fPurchaseHoldProgress + safeDelta / PURCHASE_HOLD_DURATION);
	}
	else
	{
		m_fPurchaseHoldProgress = 0.f;
	}

	// Keep completion timing linear, but reveal the gauge with EaseOutQuad.
	const _float inverseProgress = 1.f - m_fPurchaseHoldProgress;
	const _float easedProgress = 1.f - inverseProgress * inverseProgress;
	gauge->SetPathProgress(easedProgress);
	if (m_fPurchaseHoldProgress >= 1.f)
		CompletePurchase(manager);
}

void CWandShop::RefreshPurchaseCoinText()
{
	if (!m_PurchaseCoinText)
		return;

	auto* textBox = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextBox>(*m_PurchaseCoinText);
	if (!textBox)
	{
		m_PurchaseCoinText.reset();
		return;
	}

	const uint32_t coinCount =
		GET_SINGLE(UIManager)->GetRaceMiniGameCoinCount();
	if (m_iDisplayedCoinCount == coinCount)
		return;

	m_iDisplayedCoinCount = coinCount;
	textBox->SetwText(std::to_wstring(coinCount));
}

void CWandShop::SetPurchasePromptVisible(_bool visible)
{
	const auto SetActiveRecursive = [](
		auto&& self, CHandle handle, _bool active) -> void
	{
		auto* ui = GetSafeUI(handle);
		if (!ui)
			return;

		ui->SetActive(active);
		for (const CHandle childHandle : ui->GetChildren())
			self(self, childHandle, active);
	};

	for (const CHandle handle : m_PurchasePromptRoots)
		SetActiveRecursive(SetActiveRecursive, handle, visible);

	if (visible)
	{
		RefreshPurchaseCoinText();
		return;
	}

	m_fPurchaseHoldProgress = 0.f;
	if (!m_PurchaseGauge)
		return;
	if (auto* gauge = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(*m_PurchaseGauge))
	{
		gauge->SetPathProgress(0.f);
	}
}

void CWandShop::CompletePurchase(UIManager& manager)
{

	if (const auto* playerLayer =
		E::CGameInstance::Get().GetGameObjectLayer("03_Player"))
	{
		for (const CHandle playerHandle : *playerLayer)
		{
			auto* player = E::CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer>(playerHandle);
			if (!player)
				continue;

			auto* weapon = E::CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer_Weapon>(
					player->GetWeaponHandle());
			if (weapon && SUCCEEDED(weapon->EquipWand2()))
				manager.SetPurchasedWandEquipped(true);

			break;
		}
	}

	if (auto* soundManager = E::CGameInstance::Get().GetSoundManager())
	{
		soundManager->Play2D(
			"./Resources/SampleClient/Sound/UI/Purchase.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::UI,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			});
	}

	m_bPurchaseCompleted = true;
	Close(manager);
}

_bool CWandShop::ConsumePurchaseCompleted()
{
	const _bool completed = m_bPurchaseCompleted;
	m_bPurchaseCompleted = false;
	return completed;
}

void CWandShop::Close(UIManager& manager)
{
	E::CGameInstance::Get().ClearUI3DPanel();
	manager.m_bWandShopWorldMode = false;
	auto deleteRoots = [&manager](std::vector<CHandle>& roots)
	{
		for (const CHandle handle : roots)
		{
			if (GetSafeUI(handle))
				manager.DeleteUIRecursive(handle);
		}
		roots.clear();
	};

	deleteRoots(m_PurchasePromptRoots);
	deleteRoots(m_PageRoots);
	deleteRoots(m_CommonRoots);
	m_PurchaseGauge.reset();
	m_PurchaseCoinText.reset();
	m_iDisplayedCoinCount = UINT32_MAX;
	m_fPurchaseHoldProgress = 0.f;
	m_iCurrentPage = UINT32_MAX;
	m_iRootWeightOffset = 0;
}

void CWandShop::AdoptEditorPageRoots()
{
	const auto* uiHandles =
		E::CGameInstance::Get().GetGameObjectLayer("Layer_UI");
	if (!uiHandles)
		return;

	std::vector<CHandle> roots;
	for (const CHandle handle : *uiHandles)
	{
		auto* ui = GetSafeUI(handle);
		if (ui && !ui->GetParent())
			roots.push_back(handle);
	}
	ClassifyRoots(roots);
}

_bool CWandShop::HasLivePageRoots() const
{
	return std::ranges::any_of(m_PageRoots,
		[](CHandle handle) { return GetSafeUI(handle) != nullptr; });
}

void CWandShop::PlayPageFadeIn(
	const std::vector<CHandle>& roots,
	_float duration) const
{
	duration = std::max(0.f, duration);
	for (const CHandle handle : roots)
	{
		auto* ui = GetSafeUI(handle);
		if (!ui)
			continue;
		const _float targetAlpha = ui->GetAlpha();
		ui->SetAlpha(0.f);

		// FillButton의 Button/CoinImage 루트는 마우스 입력을 막기 위해
		// InputLock 상태다. ButtonComponent는 InputLock이면 APPEAR 상태도
		// 전달하지 않으므로 이 경우에는 로드 직후 Tween을 직접 시작한다.
		if (ui->GetInputLcok())
		{
			if (auto* tween = ui->GetTweenCom())
			{
				tween->ClearTweens();
				tween->PlayTween(0.f, targetAlpha, duration,
					[handle](_float value)
					{
						if (auto* current = GetSafeUI(handle))
							current->SetAlpha(value);
					}, nullptr, EEaseType::EaseOutQuad);
			}
			else
			{
				ui->SetAlpha(targetAlpha);
			}
			continue;
		}

		ui->Appear = [handle, targetAlpha, duration](CUIObject*)
		{
			auto* target = GetSafeUI(handle);
			if (!target)
				return;
			target->SetAlpha(0.f);
			if (auto* tween = target->GetTweenCom())
			{
				tween->PlayTween(0.f, targetAlpha, duration,
					[handle](_float value)
					{
						if (auto* current = GetSafeUI(handle))
							current->SetAlpha(value);
					}, nullptr, EEaseType::EaseOutQuad);
			}
			else
			{
				target->SetAlpha(targetAlpha);
			}
		};
	}
}

void CWandShop::OpenPage(UIManager& manager, uint32_t pageIndex)
{
	pageIndex = std::min<uint32_t>(pageIndex,
		static_cast<uint32_t>(PAGE_PREFABS.size() - 1u));
	if (m_iCurrentPage == pageIndex && HasLivePageRoots())
		return;

	const _bool hasLiveCommonRoots = std::ranges::any_of(m_CommonRoots,
		[](CHandle handle) { return GetSafeUI(handle) != nullptr; });
	if (!hasLiveCommonRoots &&
		E::CGameInstance::Get().GetCurrentLevelID() == ETOUI(LEVEL::UIEDITOR))
	{
		AdoptEditorPageRoots();
	}

	for (const CHandle handle : m_PageRoots)
	{
		if (GetSafeUI(handle))
			manager.DeleteUIRecursive(handle);
	}
	m_PageRoots.clear();

	m_iCurrentPage = pageIndex;
	CGeneralButton::SetCurrentWandShopPage(pageIndex);
	SetPurchasePromptVisible(pageIndex == 0u);
	m_PageRoots = manager.LoadPrefabFiltered(
		std::string(PAGE_PREFABS[pageIndex]),
		"./Resources/SampleClient/UIData/RTT/",
		[](const nlohmann::ordered_json& object)
		{
			return !CWandShop::IsCommonRootName(
				object.value("Name", std::string{}));
		});
	ApplyRootWeightOffset(m_PageRoots, false);
	CGeneralButton::RefreshWandShopCommonUI(pageIndex);
	PlayPageFadeIn(m_PageRoots, PAGE_FADE_IN_DURATION);
}
