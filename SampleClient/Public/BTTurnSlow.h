#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTTurnSlow final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTTurnSlow, CBTActionNode)
private:
	CBTTurnSlow();
	CBTTurnSlow(const CBTTurnSlow& rhs);
	~CBTTurnSlow() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitalizePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg)override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;
	virtual void		Update_Gui() override;

public:
	static UPtr<CBTTurnSlow> Create();
	UPtr<CBTRoot> Clone(void* pArg)override;
};
NS_END
