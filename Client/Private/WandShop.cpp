#include "pch.h"
#include "WandShop.h"

#include "GeneralButton.h"
#include "GameInstance.h"
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
	m_fPurchaseHoldProgress = 0.f;

	auto promptRoots = GET_SINGLE(UIManager)->LoadPrefab(
		"FillButton",
		"./Resources/SampleClient/UIData/RTT/");
	if (promptRoots.empty())
		return;

	CUIObject* button = nullptr;
	CTextureUI* gauge = nullptr;
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
			child->SetInputLcok(true);
			if (std::string_view(child->GetName()) == "Text")
			{
				child->GetUIInfo().Weight = PURCHASE_CHILD_WEIGHT;
				continue;
			}
			if (std::string_view(child->GetName()) != "BTFrame")
				continue;
			gauge = E::CGameInstance::Get().
				GetGameObjectByHandleT<CTextureUI>(childHandle);
			if (gauge)
			{
				m_PurchaseGauge = childHandle;
				gauge->GetUIInfo().Weight = PURCHASE_CHILD_WEIGHT;
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
}

void CWandShop::Update(UIManager& manager, _float fTimeDelta)
{
	if (!IsOpen() || !m_PurchaseGauge)
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

void CWandShop::CompletePurchase(UIManager& manager)
{
	// The selected wand model replacement can be connected here later.
	Close(manager);
}

void CWandShop::Close(UIManager& manager)
{
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

void CWandShop::PlayPageFadeIn(const std::vector<CHandle>& roots) const
{
	for (const CHandle handle : roots)
	{
		auto* ui = GetSafeUI(handle);
		if (!ui)
			continue;
		const _float targetAlpha = ui->GetAlpha();
		ui->SetAlpha(0.f);
		ui->Appear = [handle, targetAlpha](CUIObject*)
		{
			auto* target = GetSafeUI(handle);
			if (!target)
				return;
			target->SetAlpha(0.f);
			if (auto* tween = target->GetTweenCom())
			{
				tween->PlayTween(0.f, targetAlpha, PAGE_FADE_IN_DURATION,
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
	PlayPageFadeIn(m_PageRoots);
}
