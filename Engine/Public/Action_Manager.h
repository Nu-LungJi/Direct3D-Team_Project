#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CAction_Manager final : public CEngineBase
{
 
public:
	DECLARE_DERIVED_TYPE(CAction_Manager, CEngineBase)
private:
	explicit CAction_Manager();
	~CAction_Manager() override;

private:
	HRESULT					Initialize();
	UPtr<class CBTRoot>	    Clone_Action(BEHAVIOR eType, const _string& strActionName, void* pArg);
public:
	void					Show_Action_NodeWidget(CBTRoot* pNode);
	HRESULT					Add_Action_Prototype(BEHAVIOR eType, const _string& strActionName, UPtr<class CBTRoot> pAction);
	UPtr<class CBTRoot>		Show_ActioNode_List(BEHAVIOR eType, uint32_t& iNode,ImVec2 vNodePos, CHandle Handle);
private:
	std::map<_string, UPtr<class CBTRoot>>			m_Prototype_Actions[ETOUI(BEHAVIOR::END)];

	_string												m_SelectName{};
	_bool												m_bPopup{ false };
	_bool												m_bNode[ETOUI(NODE_ACTION::END)]{ false };
public:

	static UPtr<CAction_Manager> Create();

};

NS_END