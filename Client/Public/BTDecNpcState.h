#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"
#include "BlackBoardKey.h"
#include "NpcMom.h"
NS_BEGIN(Client)
class CBTDecNpcState final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecNpcState, CBTDecorator)
private:
	CBTDecNpcState();
	CBTDecNpcState(const CBTDecNpcState& rhs);
	~CBTDecNpcState() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	NPC_STATE					m_eState{ NPC_STATE::END };
public:
	static UPtr<CBTDecNpcState> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

