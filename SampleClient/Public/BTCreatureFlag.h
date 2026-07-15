#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTCreatureFlag final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTCreatureFlag, CBTActionNode)
private:
	CBTCreatureFlag();
	CBTCreatureFlag(const CBTCreatureFlag& rhs);
	~CBTCreatureFlag() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;
	virtual void		Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	uint32_t						m_iFlag{};
	FLAGTYPE						m_eType{FLAGTYPE::RESET};
public:
	static UPtr<CBTCreatureFlag> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

