#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecSearch final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecSearch, CBTDecorator)
private:
	CBTDecSearch();
	CBTDecSearch(const CBTDecSearch& rhs);
	~CBTDecSearch() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	virtual nlohmann::json			Save_Node()override;
	HRESULT							Load_json(const nlohmann::json& j) override;
	EVALUATE						Evaluate(_float fTimeDelta) override;
	virtual void					Update_Gui() override;
private:
	void						Abort() override;
	void						OnEnter() override;
	void						OnExit(EVALUATE eResult) override;
private:
	_float				m_fDis{10.f};
	_bool				m_bTrue{ false }, m_bRunning{ false }, m_bInvert{ false };
	EVALUATE			m_PreEval{ EVALUATE::SUCCESS };
public:
	static UPtr<CBTDecSearch> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

