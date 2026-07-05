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
	UPtr<class CBTRoot>	    Clone_Action(const _string& strActionName, void* pArg);
private:
	void					PopupAnimation(class CComAnimator* pAnimator, class  CBTActionNode* pNode);
public:
	void					Show_Action_NodeWidget(CBTRoot* pNode);
	HRESULT					Add_Action_Prototype(const _string& strActionName, UPtr<class CBTRoot> pAction);
	UPtr<class CBTRoot>		Show_ActioNode_List(uint32_t& iNode,ImVec2 vNodePos, CHandle Handle);
private:
	std::map<_string, UPtr<class CBTRoot>>				m_Prototype_Actions;

	_string												m_SelectName{};
	_bool												m_bPopup{ false };
public:

	static UPtr<CAction_Manager> Create();

};

NS_END