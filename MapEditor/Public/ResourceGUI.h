#pragma once
#include "GUIWindow.h"

NS_BEGIN(Client)

class CModelThumbnailCache;

class CResourceGUI : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CResourceGUI, CGUIWindow)

private:
	CResourceGUI();
	~CResourceGUI() override;

public:
	void UpdateGUI(E::_float fTimeDelta) override;

public:
	static E::UPtr<CResourceGUI> Create(E::CHandle* pSelectedObject);

private:
	char m_SearchBuffer[128]{};
	int m_SelectedCategory{};
	E::UPtr<CModelThumbnailCache> m_pThumbnailCache{};

};

NS_END
