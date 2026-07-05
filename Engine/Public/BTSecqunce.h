#pragma once
#include "BTComposite.h"

NS_BEGIN(Engine)
class  CBTSecqunce final : public CBTComposite
{
public:
	DECLARE_DERIVED_TYPE(CBTSecqunce, CBTComposite)
public:
	typedef struct tagbtsecqunce : CBTRoot::BTROOT_DESC
	{

	}BTSECQUNCE_DESC;

private:
	explicit CBTSecqunce();
	~CBTSecqunce() override;

	HRESULT Initalize(void* pArg) override;

public:
	virtual HRESULT	Priority_Update(_float fTimeDelta) override;
	virtual HRESULT	Update(_float fTimeDelta)		   override;
	virtual HRESULT	Late_Update(_float fTimeDelta)	   override;

	virtual EVALUATE	Evaluate(_float fTimeDelta)override;

	nlohmann::json				Save_Node()override;
	HRESULT						Load_json(nlohmann::json& j) override;
public:
	static  UPtr<CBTSecqunce> Create(void* pArg);
	UPtr<CBTRoot>Clone(void* pArg) { return nullptr; };
};

NS_END