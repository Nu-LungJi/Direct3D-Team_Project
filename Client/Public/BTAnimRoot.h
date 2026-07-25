#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"
#include "Monster.h"
NS_BEGIN(Client)
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
protected:
	_bool				m_bLoop{ true }, m_bStart{ true }, m_bRatio{ false };

	ATTMON				m_eSkillType{ ATTMON::END };
	_float2				m_fSkillRatio{}, m_fRatio{};
	uint32_t			m_iEndFlag{}, m_iStartFlag{}, m_iLoopCnt{ 0 };
public:
	static UPtr<CBTAnimRoot> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END
