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

	}ACTION_NODE_DESC;

protected:
	explicit CBTActionNode();
	explicit CBTActionNode(const CBTActionNode& Prototype);
	~CBTActionNode();

	virtual HRESULT Initalize(void* pArg) override;

public:
	virtual EVALUATE		Evaluate() PURE;

};

NS_END