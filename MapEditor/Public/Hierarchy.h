#pragma once
#include "GUIWindow.h"

NS_BEGIN(Client)

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
	static E::UPtr<CHierarchy> Create(E::CHandle* pSelectedObject);
};

NS_END
