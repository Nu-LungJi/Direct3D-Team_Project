#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"
#include "Monster.h"
NS_BEGIN(Client)
typedef struct animflag
{
	_bool bFlag{ false };
	_float fRatio{0};
	FLAGTYPE eType{ FLAGTYPE::RESET};
	uint32_t iFlag{0};
}FLAG_EVENT;
class CBTAnimRoot : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTAnimRoot, CBTActionNode)
protected:
	CBTAnimRoot();

	CBTAnimRoot(const CBTAnimRoot& rhs);
	~CBTAnimRoot() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT	InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initalize(void* pArg)override;
public:
	EVALUATE				Evaluate(_float fTimeDelta) override { return EVALUATE::SUCCESS; };
	virtual void			Update_Gui() override;
	void					Abort() override;
	virtual nlohmann::json	Save_Node()override;
	HRESULT					Load_json(const nlohmann::json& j) override;
	
protected:
	void				Active_Skill();
	void				EventFlagToRatio(_float fRatio);
protected:
	void				Reset_CheckFlag();
private:
	//GUi
	void				Combo(const _char* pName,uint32_t& iFlag);
	void				Combo2(const _char* pName, FLAGTYPE& eType);
protected:
	_bool						m_bLoop{ true }, m_bStart{ true }, m_bRatio{ false };

	ATTMON						m_eSkillType{ ATTMON::END };
	_float2						m_fSkillRatio{}, m_fRatio{};
	_float					     m_fBlend{ 0.1f };
	uint32_t					m_iLoopCnt{ 0 };
	std::vector<FLAG_EVENT>		m_StartFlags{};
	std::vector<FLAG_EVENT>		m_EndFlags{};
private:
	uint32_t					m_iStartFlagCheck{};
	FLAG_EVENT					m_AddFlag{};
public:
	static UPtr<CBTAnimRoot> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END
