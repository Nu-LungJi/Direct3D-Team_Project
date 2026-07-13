#pragma once
#include "GUIWindow.h"

NS_BEGIN(Client)

class CEditorCommandManager;

class CHierarchy : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CHierarchy, CGUIWindow)

private:
	CHierarchy();
	~CHierarchy();

public:
	virtual void UpdateGUI(E::_float fTimeDelta) override;

public:
	static E::UPtr<CHierarchy> Create(E::CHandle* pSelectedObject,
		CEditorCommandManager* pCommandManager);

private:
	CEditorCommandManager* m_pCommandManager = nullptr;
};

NS_END
