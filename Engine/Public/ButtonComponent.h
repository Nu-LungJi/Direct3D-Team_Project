#pragma once

#include "UIComponent.h"

NS_BEGIN(Engine)

class ENGINE_DLL CButtonComponent : public CUIComponent
{
public:
	DECLARE_DERIVED_TYPE(CButtonComponent, CUIComponent)

private:
	explicit CButtonComponent();
	~CButtonComponent() override;

public:
	bool CheckPixelPerfectCollision(_float2 mousePos, bool bIsTopUI);

private:
	bool PtInRect(_float2 mousePos);
private:
	bool m_bIsHovered = false;
	bool m_bAppear = true;
	bool m_bDisppear = false;

public:
	virtual HRESULT		Initialize(void* pArg) override;
	virtual void		Update(_float fTimeDelta, _float2 mousePos);

public:
	void SetDisappear(_bool disa) { m_bDisppear = disa; }

public:
	static UPtr<CButtonComponent> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
