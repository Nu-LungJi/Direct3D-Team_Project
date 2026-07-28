#pragma once
#include "Client_Defines.h"
#include "BTAnimRoot.h"
NS_BEGIN(Client)

class CBTHitAnimMonster final : public CBTAnimRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTHitAnimMonster, CBTAnimRoot)
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
	void				ComboAttMon(const _char* pName, ATTMON& eTye);
	void				ComboHit(const _char* pName, PLAYER_SKILL_TYPE& eTye);
	void				ComboAnim(const _char* pName,  int32_t& iAnimIndex, uint32_t iArrayIndex);
	
private:
	uint32_t			m_iTable{};
	MOVE				m_eMove{ MOVE::STRAIGHT };
	_bool				m_bRatioInvert{ false }, m_bInterrupt{ false };

	std::vector<HITTABLE>		m_HitTable;
		
	uint32_t					m_iArrayIndex{UINT_MAX};
public:
	static UPtr<CBTHitAnimMonster> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END
