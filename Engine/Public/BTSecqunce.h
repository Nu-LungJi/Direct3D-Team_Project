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
	CBTSecqunce(const CBTSecqunce& rhs);
	~CBTSecqunce() override;

	HRESULT	InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;

	void OnEnter() override;
	void OnExit(EVALUATE eResult) override;
public:

	virtual EVALUATE			Evaluate(_float fTimeDelta)override;
	void						Abort() override;
	nlohmann::json 				Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
public:
	static  UPtr<CBTSecqunce> Create(void* pArg);
	UPtr<CPrototype>Clone(void* pArg) override ;
};

NS_END
