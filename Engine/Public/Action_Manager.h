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
public:
	void					Show_Action_NodeWidget(CBTRoot* pNode);
	UPtr<class CBTRoot>		Show_ActioNode_List(NODEGROUP eType, uint32_t& iNode,ImVec2 vNodePos, CHandle Handle);
private:
	std::map<_string, UPtr<class CBTRoot>>			m_Prototype_Actions[ETOUI(NODEGROUP::END)];

	_string												m_SelectName{};
	_bool												m_bPopup{ false };
	_bool												m_bNode[ETOUI(NODEGROUP::END)]{ false };
public:

	static UPtr<CAction_Manager> Create();

};

NS_END
