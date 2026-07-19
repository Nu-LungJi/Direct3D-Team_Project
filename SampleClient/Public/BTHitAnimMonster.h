#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"
#include "TestGob.h"
NS_BEGIN(Client)

class CBTHitAnimMonster final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTHitAnimMonster, CBTActionNode)
private:
	CBTHitAnimMonster();

	CBTHitAnimMonster(const CBTHitAnimMonster& rhs);
	~CBTHitAnimMonster() override;
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
	_bool							HitType();
private:
	//Gui
	uint32_t			m_iTable{};
private:

	MOVE				m_eMove{ MOVE::STRAIGHT };
	_float2				m_fRatio{  };
	uint32_t			m_iLoopCnt{ 0 }, m_iStartFlag{}, m_iEndFlag{};
	_bool				m_bLoop{ true }, m_bStart{ true }, m_bRatio{ false }, m_bRatioInvert{ false };

	HITTABLE			m_HitTable[ETOUI(HITMON::END)];
	int32_t				m_iHitAnim[ETOUI(HITMON::END)];
public:
	static UPtr<CBTHitAnimMonster> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END
