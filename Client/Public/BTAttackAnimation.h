#pragma once
#include "Client_Defines.h"
#include "BTAnimRoot.h"

NS_BEGIN(Client)
class CBTAttackAnimation final : public CBTAnimRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTAttackAnimation, CBTAnimRoot)
private:
	CBTAttackAnimation();

	CBTAttackAnimation(const CBTAttackAnimation& rhs);
	~CBTAttackAnimation() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT							InitializePrototype(void* pArg = nullptr) override;
	HRESULT							Initalize(void* pArg)override;
public:
	EVALUATE						Evaluate(_float fTimeDelta) override;
	virtual void					Update_Gui() override;
	void							Abort() override;
	virtual nlohmann::json			Save_Node()override;
	HRESULT							Load_json(const nlohmann::json& j) override;
private:
	MOVE				m_eMove{ MOVE::STRAIGHT };

	_float				m_fDis{}, m_fTime{};
	uint32_t		    m_iStartFlag{}, m_iEndFlag{};
	_bool				m_bRatioInvert{ false };
public:
	static UPtr<CBTAttackAnimation> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END
