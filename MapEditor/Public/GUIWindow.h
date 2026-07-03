#pragma once
#include "Engine_Defines.h"
#include "Client_Defines.h"
#include "Handle.h"
#include "GameObject.h"

NS_BEGIN(Client)

class CGUIWindow abstract : public E::CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CGUIWindow, CEngineBase)

protected:
	CGUIWindow();
	virtual ~CGUIWindow();

public:
	virtual void UpdateGUI(E::_float fTimeDelta) = 0;

protected:
	HRESULT Initialize(E::CHandle* pSelectedObject);
	E::CHandle* GetSelectedHandle() const { return m_pSelectedObject; }
	E::CGameObject* GetSelectedObject() const;

private:
	E::CHandle* m_pSelectedObject{};
};

NS_END
