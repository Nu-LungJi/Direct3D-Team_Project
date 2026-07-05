#pragma once
#include "BTRoot.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBTActionNode  : public CBTRoot 
{
public:
	DECLARE_DERIVED_TYPE(CBTActionNode, CBTRoot)
	CBTActionNode& operator=(const CBTActionNode&) = delete;

public:
	typedef struct tagcombtactionnode : CBTRoot::BTROOT_DESC
	{
		ACTION_VALUE Value{};
	}ACTION_NODE_DESC;

protected:
	explicit CBTActionNode();
	explicit CBTActionNode(const CBTActionNode& Prototype);
	~CBTActionNode();

	virtual HRESULT Initalize(void* pArg) override;
public:
	HRESULT	Priority_Update(_float fTimeDelta) override;
	HRESULT	Update(_float fTimeDelta)		   override;
	HRESULT	Late_Update(_float fTimeDelta)	   override;

	const ACTION_VALUE		Get_Value() { return m_Value; }
public:
	virtual EVALUATE		Evaluate() PURE;
private:
	ACTION_VALUE			m_Value{};
public:
	virtual UPtr<CBTRoot> Clone(void* pArg) PURE;
};

NS_END