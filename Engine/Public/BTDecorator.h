#pragma once
#include "BTComposite.h"

NS_BEGIN(Engine)
class  ENGINE_DLL CBTDecorator : public CBTComposite
{
public:
	DECLARE_DERIVED_TYPE(CBTDecorator, CBTComposite)
	CBTDecorator& operator=(const CBTDecorator&) = delete;
public:
	typedef struct tagdecorator : CBTRoot::BTROOT_DESC
	{

	}DECORATOR_DESC;

protected:
	explicit CBTDecorator();
	CBTDecorator(const CBTDecorator& Prototype);
	~CBTDecorator() override;

	virtual HRESULT Initalize(void* pArg) override;

public:
	virtual HRESULT	Priority_Update(_float fTimeDelta) override;
	virtual HRESULT	Update(_float fTimeDelta)		   override;
	virtual HRESULT	Late_Update(_float fTimeDelta)	   override;

	virtual EVALUATE	Evaluate(_float fTimeDelta)override;

	nlohmann::json				Save_Node()override;
	HRESULT						Load_json(nlohmann::json& j)override;
public:
	virtual UPtr<CBTRoot>Clone(void* pArg) PURE;
};

NS_END