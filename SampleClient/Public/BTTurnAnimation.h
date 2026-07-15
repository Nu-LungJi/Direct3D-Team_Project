#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTTurnAnimation final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTTurnAnimation, CBTActionNode)
private:
	CBTTurnAnimation();

	CBTTurnAnimation(const CBTTurnAnimation& rhs);
	~CBTTurnAnimation() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT	InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initalize(void* pArg)override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	_bool				m_bLoop{ true }, m_bStart{ true };
	int32_t				m_iTurnAnimIndex[ETOUI(TURN::END)];
public:
	static UPtr<CBTTurnAnimation> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END
