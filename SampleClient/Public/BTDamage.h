#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTDamage final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTDamage, CBTActionNode)
private:
	CBTDamage();
	CBTDamage(const CBTDamage& rhs);
	~CBTDamage() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	virtual nlohmann::json	 Save_Node()override;
	HRESULT					 Load_json(const nlohmann::json& j) override;
	EVALUATE				 Evaluate(_float fTimeDelta) override;
	virtual void			 Update_Gui() override;
public:
	static UPtr<CBTDamage> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

