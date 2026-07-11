#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecLier final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecLier, CBTDecorator)
private:
	CBTDecLier();
	CBTDecLier(const CBTDecLier& rhs);
	~CBTDecLier() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE			 Evaluate(_float fTimeDelta) override;

	virtual void		Update_Gui() override;
private:
	_bool				m_bEnter{ false };
public:
	static UPtr<CBTDecLier> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

