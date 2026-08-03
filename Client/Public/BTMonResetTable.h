#pragma once
#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTMonResetTable final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTMonResetTable, CBTActionNode)
private:
	CBTMonResetTable();
	CBTMonResetTable(const CBTMonResetTable& rhs);
	~CBTMonResetTable() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;

	virtual void		Update_Gui() override;
	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	_bool			m_bHardReset{ false };
public:
	static UPtr<CBTMonResetTable> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

