#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"
#include "BlackBoardKey.h"
#include "WorldAgent.h"
NS_BEGIN(Client)
class CBTChangeState final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTChangeState, CBTActionNode)
private:
	CBTChangeState();
	CBTChangeState(const CBTChangeState& rhs);
	~CBTChangeState() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	AGENT_STATE					m_eState{ AGENT_STATE::END };
public:
	static UPtr<CBTChangeState> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

