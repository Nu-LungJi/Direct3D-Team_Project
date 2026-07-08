#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecLier final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecLier, CBTDecorator)
private:
	CBTDecLier();
	CBTDecLier(const CBTDecLier& rhs);
	~CBTDecLier() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitalizePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;
	virtual nlohmann::json			Save_Node()override;
	HRESULT					Load_json(const nlohmann::json& j) override;

	virtual void		Update_Gui() override;
private:
	_float				m_fDist{ 10.f };
public:
	static UPtr<CBTDecLier> Create();
	UPtr<CBTRoot> Clone(void* pArg)override;
};
NS_END

