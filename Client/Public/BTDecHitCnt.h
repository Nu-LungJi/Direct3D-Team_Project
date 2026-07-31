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
	void							Abort() override;
	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	int32_t					m_iMaxHitCnt{};
public:
	static UPtr<CBTDecHitCnt> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

