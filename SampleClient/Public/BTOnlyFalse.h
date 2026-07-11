#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTOnlyFalse final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTOnlyFalse, CBTActionNode)
private:
	CBTOnlyFalse();
	CBTOnlyFalse(const CBTOnlyFalse& rhs);
	~CBTOnlyFalse() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;
	virtual void		Update_Gui() override;
private:
	MOVE						m_eMove{};
public:
	static UPtr<CBTOnlyFalse> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

