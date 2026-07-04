#pragma once
#include "BTComposite.h"

NS_BEGIN(Engine)
class  CBTSelector final : public CBTComposite
{
public:
	DECLARE_DERIVED_TYPE(CBTSelector, CBTComposite)
public:
	typedef struct tagbtselector : CBTRoot::BTROOT_DESC
	{

	}BTSELECTOR_DESC;
private:
	explicit CBTSelector();
	~CBTSelector() override;

	HRESULT Initalize(void* pArg) override;

public:
	virtual HRESULT	Priority_Update(_float fTimeDelta) override;
	virtual HRESULT	Update(_float fTimeDelta)		   override;
	virtual HRESULT	Late_Update(_float fTimeDelta)	   override;
public:					
	//HRESULT					Add_Secqunce(const _string& strSequenceName);
	//HRESULT					Add_ActionNode(int32_t iSequenceIndex, UPtr<CBTRoot> pActionNode);
	//HRESULT					Add_Selector(const _string& strSelectoreName);
	virtual EVALUATE		Evaluate() override;
public:
	static  UPtr<CBTSelector> Create(void* pArg);

};

NS_END