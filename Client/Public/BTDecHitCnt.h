#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecHitCnt final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecHitCnt, CBTDecorator)
private:
	CBTDecHitCnt();
	CBTDecHitCnt(const CBTDecHitCnt& rhs);
	~CBTDecHitCnt() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE						Evaluate(_float fTimeDelta) override;
	virtual void					Update_Gui() override;
private:
	_bool						m_bDeadCheck{ false };
	_float						m_CurrentHp{ 40 }, m_MaxHp{ 100 }, m_fdivided{ 1.f };
public:
	static UPtr<CBTDecHitCnt> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

