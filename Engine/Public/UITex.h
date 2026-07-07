#pragma once

#include "GameObject.h"
#include "UIObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CUITex : public CUIObject
{
public:
	DECLARE_DERIVED_TYPE(CUITex, CUIObject)

protected:
	CUITex();
	~CUITex() override;

public:
	HRESULT Initialize(void* pArg) override;
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);

protected:
	virtual void Creating();
	virtual void StartHovering();
	virtual void Hovering();
	virtual void EndHovering();
	virtual void Ending();
};

NS_END

