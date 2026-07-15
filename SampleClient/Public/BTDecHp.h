#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecHp final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecHp, CBTDecorator)
private:
	CBTDecHp();
	CBTDecHp(const CBTDecHp& rhs);
	~CBTDecHp() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE				 Evaluate(_float fTimeDelta) override;
	virtual void			 Update_Gui() override;
private:
	_float						m_CurrentHp{ 40 }, m_MaxHp{100};
public:
	static UPtr<CBTDecHp> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

