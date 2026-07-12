#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDead final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDead, CBTDecorator)
private:
	CBTDead();
	CBTDead(const CBTDead& rhs);
	~CBTDead() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT	InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;
	virtual nlohmann::json			Save_Node()override;
	HRESULT					Load_json(const nlohmann::json& j) override;

	virtual void		Update_Gui() override;
private:
	_float				m_fDist{ 10.f };
public:
	static UPtr<CBTDead> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

