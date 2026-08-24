#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

NS_BEGIN(Client)
class CBTDecHitPlayer final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecHitPlayer, CBTDecorator)
private:
	CBTDecHitPlayer();
	CBTDecHitPlayer(const CBTDecHitPlayer& rhs);
	~CBTDecHitPlayer() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;

	_bool	bHitCheckPlayer();
public:
	EVALUATE						Evaluate(_float fTimeDelta) override;
	virtual void					Update_Gui() override;
	void							Abort() override;
	virtual nlohmann::json			Save_Node()override;
	HRESULT							Load_json(const nlohmann::json& j) override;
private:
	int32_t					m_iMaxHitCnt{};
public:
	static UPtr<CBTDecHitPlayer> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

