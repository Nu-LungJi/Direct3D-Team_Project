#pragma once
#include "GUIWindow.h"

NS_BEGIN(Client)

class CInspector : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CInspector, CGUIWindow)

private:
	CInspector();
	~CInspector();

public:
	virtual void UpdateGUI(E::_float fTimeDelta) override;

public:
	static E::UPtr<CInspector> Create(E::CHandle* pSelectedObject);
};

NS_END
