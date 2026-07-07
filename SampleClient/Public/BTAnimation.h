#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTAnimation final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTAnimation, CBTActionNode)
private:
	CBTAnimation();

	CBTAnimation(const CBTAnimation& rhs);
	~CBTAnimation() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitalizePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg)override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;

	virtual void		Update_Gui() override;
public:
	static UPtr<CBTAnimation> Create();
	UPtr<CBTRoot> Clone(void* pArg)override;
};
NS_END