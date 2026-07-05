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
	virtual EVALUATE		Evaluate(_float fTimeDelta) override;

	nlohmann::json				Save_Node()override;
	HRESULT						Load_json(nlohmann::json& j) override;
public:
	static  UPtr<CBTSelector> Create(void* pArg);
	UPtr<CBTRoot>Clone(void* pArg) {return nullptr;}
};

NS_END