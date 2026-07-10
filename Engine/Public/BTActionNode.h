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
	CBTActionNode(const CBTActionNode& pPrototype);
	~CBTActionNode() override;

	virtual HRESULT	InitalizePrototype(void* pArg) override;
	virtual HRESULT Initalize(void* pArg) override;
public:
	virtual void		Update_Gui();
	ACTION_VALUE&		Get_Value() { return m_Value; }
public:
	virtual EVALUATE			Evaluate(_float fTimeDelta) PURE;
	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
protected:
	ACTION_VALUE			m_Value{};
	_bool					m_bPopup{ false };
public:
	virtual UPtr<CBTRoot> Clone(void* pArg) PURE;
};

NS_END
