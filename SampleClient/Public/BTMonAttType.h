#pragma once
#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTMonAttType final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTMonAttType, CBTActionNode)
private:
	CBTMonAttType();
	CBTMonAttType(const CBTMonAttType& rhs);
	~CBTMonAttType() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;
	virtual void		Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	ATTMON						m_eAttType{ ATTMON::END };
public:
	static UPtr<CBTMonAttType> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

