#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)
//using namespace ax;

class CNodeEditor : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CNodeEditor, CEngineBase)
protected:
	explicit CNodeEditor();
	~CNodeEditor()override;

private:
	HRESULT Initialize();
public:
	void UpdateGUI();
	void RenderGUI();
	void NodeEditorUpdate();
private:
	void	   LeftSide_MenuBar();
	void	   NodeList_Panel(int32_t* piNode_hoverd_List, _bool* pbContext_Manu);
	void	   Begin_Canvas();
	void	   Draw_Grid();
	void	   Draw_Link();
	void	   Draw_Node(int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io);
	
	int32_t    Choice_EndSlot(GUINODE* pNode, const _float& fNode_Radius);
	int32_t    Choice_StartSlot(GUINODE* pNode, const _float& fNode_Radius);

	void    End_Canvas();

private:
	//±â´É
	void	Reset_CurrentNode();
	void	Draw_NodeLine(_float2 iStartnode, _float2 iEndNode,_bool bMouse = false);
	_bool	ImsMouseHoverSlot(_float2 vSlotPos, const _float& fNode_Radius);
private:
	ax::NodeEditor::EditorContext* m_pNodeContext{ nullptr };
	std::vector<GUINODE>		m_Nodes;
	std::vector<GUINODE_LINK>	m_NodesLink;
	
	ImDrawList*					m_pDrawList{ nullptr };
	GUICURRENT_NODE				m_CurrentNode{ nullptr };

	_float2						m_vScroll{ 0,0 }, m_vOffset{};
	_bool						m_binited{ false }, m_bShow_grid{ true };
	
	int32_t						m_iNodeSelect{ -1 };
public:
	static UPtr<CNodeEditor> Create();
	
	
};

NS_END

