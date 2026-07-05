#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTMove final : public CBTActionNode
{ 
public:
	DECLARE_DERIVED_TYPE(CBTMove, CBTActionNode)
private:
	explicit CBTMove();
	~CBTMove() override;
	// CBTActionNode을(를) 통해 상속됨
	HRESULT Initalize(void* pArg) override;
public:
	HRESULT Priority_Update(_float fTimeDelta) override;
	HRESULT Update(_float fTimeDelta) override;
	HRESULT Late_Update(_float fTimeDelta) override;
	EVALUATE Evaluate() override;

public:
	static UPtr<CBTMove> Create();
};
NS_END

