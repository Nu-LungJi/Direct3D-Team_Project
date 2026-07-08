#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecInvert final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecInvert, CBTDecorator)
private:
	CBTDecInvert();
	CBTDecInvert(const CBTDecInvert& rhs);
	~CBTDecInvert() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitalizePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE			 Evaluate(_float fTimeDelta) override;

	virtual void		Update_Gui() override;
private:
	_bool				m_bEnter{ false };
public:
	static UPtr<CBTDecInvert> Create();
	UPtr<CBTRoot> Clone(void* pArg)override;
};
NS_END

