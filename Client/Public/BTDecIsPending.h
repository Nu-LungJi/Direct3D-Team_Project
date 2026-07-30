#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecIsPending final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecIsPending, CBTDecorator)
private:
	CBTDecIsPending();
	CBTDecIsPending(const CBTDecIsPending& rhs);
	~CBTDecIsPending() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override;
private:
	MOVE						m_eMove{};
	uint32_t					m_iFlag{ ETOUI(BTFLAG::HIT) };
public:
	static UPtr<CBTDecIsPending> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

