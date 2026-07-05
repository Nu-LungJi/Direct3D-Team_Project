#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTchase final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTchase, CBTActionNode)
private:
	CBTchase();
	~CBTchase() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype();
	HRESULT Initalize(void* pArg)override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;

public:
	static UPtr<CBTchase> Create();
	UPtr<CBTRoot> Clone(void* pArg)override;
};
NS_END