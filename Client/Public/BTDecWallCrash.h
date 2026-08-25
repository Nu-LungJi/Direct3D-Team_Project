#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecWallCrash final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecWallCrash, CBTDecorator)
private:
	CBTDecWallCrash();
	CBTDecWallCrash(const CBTDecWallCrash& rhs);
	~CBTDecWallCrash() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;

private:
	void						Abort() override;
	void						OnEnter() override;
private:
	_bool						Wall_Crash();
private:
	_bool						m_bCrash{false};
	_float						m_fTick{}, m_fTime{2.f};
public:
	static UPtr<CBTDecWallCrash> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

