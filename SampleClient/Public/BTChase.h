#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTChase final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTChase, CBTActionNode)
private:
	CBTChase();
	CBTChase(const CBTChase& rhs);
	~CBTChase() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitalizePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	virtual nlohmann::json	 Save_Node()override;
	HRESULT					 Load_json(const nlohmann::json& j) override;
	EVALUATE				 Evaluate(_float fTimeDelta) override;
	virtual void			 Update_Gui() override;
private:
	MOVE						m_eMove{};
public:
	static UPtr<CBTChase> Create();
	UPtr<CBTRoot> Clone(void* pArg)override;
};
NS_END

