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
	 ~CBTComposite() override;

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
	
	virtual EVALUATE		Evaluate() PURE;
	
	virtual HRESULT	Priority_Update(_float fTimeDelta)PURE;
	virtual HRESULT	Update(_float fTimeDelta)		  PURE;
	virtual HRESULT	Late_Update(_float fTimeDelta)	  PURE;
	
public:
	HRESULT		Add_Node(void* pArg = nullptr, UPtr<CBTRoot> pNode = nullptr);
protected:
	NODE_VALUE				m_NodeValue{};

	std::vector<UPtr<CBTRoot>>			  m_Actions;
};
NS_END