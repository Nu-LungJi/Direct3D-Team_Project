#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTMove final : public CBTActionNode
{ 
public:
	DECLARE_DERIVED_TYPE(CBTMove, CBTActionNode)
private:
	 CBTMove();
	 CBTMove(const CBTMove& Prototype);
	~CBTMove() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype();
	HRESULT Initalize(void* pArg);
public:
	EVALUATE Evaluate() override;

public:
	static UPtr<CBTMove> Create();
	UPtr<CBTRoot> Clone(void* pArg)override;
};
NS_END

