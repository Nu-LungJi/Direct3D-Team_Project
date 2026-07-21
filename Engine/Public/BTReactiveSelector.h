#pragma once
#include "BTComposite.h"

NS_BEGIN(Engine)
class  CBTReactiveSelector final : public CBTComposite
{
public:
	DECLARE_DERIVED_TYPE(CBTReactiveSelector, CBTComposite)
public:
	typedef struct tagbtselector : CBTRoot::BTROOT_DESC
	{

	}BTSELECTOR_DESC;
private:
	explicit CBTReactiveSelector();
	CBTReactiveSelector(const CBTReactiveSelector& rhs);
	~CBTReactiveSelector() override;
	HRESULT	InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	virtual EVALUATE		Evaluate(_float fTimeDelta) override;
	void					Abort() override;
	nlohmann::json			Save_Node() override;
	HRESULT					Load_json(const nlohmann::json& j) override;
public:
	static  UPtr<CBTReactiveSelector> Create(void* pArg);
	UPtr<CPrototype>Clone(void* pArg)override;
};

NS_END
