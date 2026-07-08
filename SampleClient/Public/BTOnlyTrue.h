#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTOnlyTrue final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTOnlyTrue, CBTActionNode)
private:
	CBTOnlyTrue();
	CBTOnlyTrue(const CBTOnlyTrue& rhs);
	~CBTOnlyTrue() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitalizePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;
	virtual void		Update_Gui() override;
private:
	MOVE						m_eMove{};
public:
	static UPtr<CBTOnlyTrue> Create();
	UPtr<CBTRoot> Clone(void* pArg)override;
};
NS_END

