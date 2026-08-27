#pragma once
#include "GUIWindow.h"

NS_BEGIN(Client)

class CEditorCommandManager;
class CEditorSelection;

class CHierarchy : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CHierarchy, CGUIWindow)

private:
	CHierarchy();
	~CHierarchy();

public:
	virtual void UpdateGUI(E::_float fTimeDelta) override;
	/*----------- 광윤 추가 -----------*/	// 태그 검색 기능
	bool ContainsIgnoreCase(std::string_view text, std::string_view pattern);
	/*---------------------------------*/
public:
	static E::UPtr<CHierarchy> Create(E::CHandle* pSelectedObject,
		CEditorCommandManager* pCommandManager, CEditorSelection* pSelection);

private:
	CEditorCommandManager* m_pCommandManager = nullptr;
	CEditorSelection* m_pSelection = nullptr;
	std::optional<E::CHandle> m_RangeAnchor{};

	/*----------- 광윤 추가 -----------*/	// 태그 검색 기능
	char m_TagSearchBuffer[128]{};
	/*---------------------------------*/
};

NS_END
