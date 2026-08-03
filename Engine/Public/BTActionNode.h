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

	 HRESULT					InitializePrototype(void* pArg) override;
	 HRESULT					Initalize(void* pArg) override;


	 virtual void OnEnter() {};
	 virtual void OnExit(EVALUATE eResult) {};
public:
	void						Abort() override;
	virtual void				Update_Gui();
	ACTION_VALUE&				Get_Value() { return m_Value; }
public:
	virtual EVALUATE			Evaluate(_float fTimeDelta) PURE;
	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
protected:
	ACTION_VALUE			m_Value{};
	_bool					m_bPopup{ false };
};

NS_END
