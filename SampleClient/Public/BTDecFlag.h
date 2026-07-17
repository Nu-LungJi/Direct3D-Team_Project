#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecFlag final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecFlag, CBTDecorator)
private:
	CBTDecFlag();
	CBTDecFlag(const CBTDecFlag& rhs);
	~CBTDecFlag() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE				 Evaluate(_float fTimeDelta) override;
	virtual void			 Update_Gui() override;

	virtual nlohmann::json			Save_Node()override;
	HRESULT					Load_json(const nlohmann::json& j) override;
private:
	MOVE						m_eMove{};
	uint32_t					m_iFlag{ETOUI( BTFLAG::HIT )};
public:
	static UPtr<CBTDecFlag> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

