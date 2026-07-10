#pragma once
#include "BTRoot.h"

//셀렉터 시퀀스용도
NS_BEGIN(Engine)

class  CBTComposite : public CBTRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTComposite, CBTRoot)
protected:
	explicit CBTComposite();
	CBTComposite(const CBTComposite& rhs);
	 ~CBTComposite() override;

	 virtual HRESULT InitalizePrototype(void* pArg = nullptr) { m_MasterName = "Root"; return S_OK; }
	 virtual HRESULT Initalize(void* pArg) override;
protected:
	typedef struct strnodevalue
	{
		_string strCurSecquenceName;
		int32_t	iCurSecquenceIndex = { -1 };
		int32_t	iPreSecquenceIndex = { -1 };
		_bool	bCur{ false };
	}NODE_VALUE;
public:
	std::vector<UPtr<CBTRoot>>* Get_Nodes() { return &m_Actions; }
	
	virtual EVALUATE		Evaluate(_float fTimeDelta) { return EVALUATE::SUCCESS; }
	void					Tick(_float fTimeDelta);
	void			ResetDebug() override;
public:
	HRESULT					Add_Node(void* pArg = nullptr, UPtr<CBTRoot> pNode = nullptr);
	virtual nlohmann::json  Save_Node() override;
	virtual HRESULT			Load_json(const nlohmann::json& j);
protected:
	NODE_VALUE				m_NodeValue{};

	std::vector<UPtr<CBTRoot>>			  m_Actions;
public:
	static  UPtr<CBTComposite> Create(void* pArg);
	virtual UPtr<CBTRoot>Clone(void* pArg) { return nullptr; }
};
NS_END
