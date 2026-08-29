#pragma once

#include "Client_Defines.h"
#include "UIObject.h"

NS_BEGIN(Client)

class UIManager;

class CWandShop final
{
public:
	static _bool IsPagePrefab(std::string_view prefabName);
	static _bool IsCommonRootName(std::string_view objectName);

	void RegisterLoadedPage(const std::string& prefabName,
		const std::vector<CHandle>& roots);
	_bool IsOpen() const;
	void CreatePurchasePrompt();
	void Update(UIManager& manager, _float fTimeDelta);
	void OpenPage(UIManager& manager, uint32_t pageIndex);
	void Close(UIManager& manager);
	_bool ConsumePurchaseCompleted();

private:
	void AdoptEditorPageRoots();
	void ClassifyRoots(const std::vector<CHandle>& roots);
	void ApplyRootWeightOffset(const std::vector<CHandle>& roots,
		_bool initializeOffset);
	void CompletePurchase(UIManager& manager);
	void PlayPageFadeIn(
		const std::vector<CHandle>& roots,
		_float duration) const;
	void RefreshPurchaseCoinText();
	void SetPurchasePromptVisible(_bool visible);
	_bool HasLivePageRoots() const;

private:
	std::vector<CHandle> m_CommonRoots{};
	std::vector<CHandle> m_PageRoots{};
	std::vector<CHandle> m_PurchasePromptRoots{};
	std::optional<CHandle> m_PurchaseGauge{};
	std::optional<CHandle> m_PurchaseCoinText{};
	uint32_t m_iCurrentPage{ UINT32_MAX };
	uint32_t m_iDisplayedCoinCount{ UINT32_MAX };
	int m_iRootWeightOffset{};
	_float m_fPurchaseHoldProgress{};
	_bool m_bPurchaseCompleted{};
};

NS_END
