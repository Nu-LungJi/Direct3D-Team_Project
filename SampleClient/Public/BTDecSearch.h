#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecSearch final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecSearch, CBTDecorator)
private:
	CBTDecSearch();
	~CBTDecSearch() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype();
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;

public:
	static UPtr<CBTDecSearch> Create();
	UPtr<CBTRoot> Clone(void* pArg)override;
};
NS_END

