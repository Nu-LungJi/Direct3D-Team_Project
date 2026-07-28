#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecIsGround final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecIsGround, CBTDecorator)
private:
	CBTDecIsGround();
	CBTDecIsGround(const CBTDecIsGround& rhs);
	~CBTDecIsGround() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override {};
public:
	static UPtr<CBTDecIsGround> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

