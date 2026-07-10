#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecHit final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecHit, CBTDecorator)
private:
	CBTDecHit();
	CBTDecHit(const CBTDecHit& rhs);
	~CBTDecHit() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	virtual nlohmann::json	 Save_Node()override;
	HRESULT					 Load_json(const nlohmann::json& j) override;
	EVALUATE				 Evaluate(_float fTimeDelta) override;
	virtual void			 Update_Gui() override;
private:
	MOVE						m_eMove{};
public:
	static UPtr<CBTDecHit> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

