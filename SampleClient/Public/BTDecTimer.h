#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecTimer final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecTimer, CBTDecorator)
private:
	CBTDecTimer();
	CBTDecTimer(const CBTDecTimer& rhs);
	~CBTDecTimer() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
	
public:
	EVALUATE						Evaluate(_float fTimeDelta) override;
	void							Abort() override;
	void							Update_Gui() override;

	virtual nlohmann::json			Save_Node()override;
	HRESULT							Load_json(const nlohmann::json& j) override;
private:
	_bool							m_bRun{ true };
	_float							m_fWaitTime{}, m_fTick{};
public:
	static UPtr<CBTDecTimer> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

