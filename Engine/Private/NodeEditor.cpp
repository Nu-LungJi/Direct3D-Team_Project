#include "pch.h"
#include "NodeEditor.h"
#include "BTSelector.h"
#include "BTSecqunce.h"
#include "BTActionNode.h"
#include "ComBeHavior.h"
CNodeEditor::CNodeEditor()
{
}

CNodeEditor::~CNodeEditor()
{
	ax::NodeEditor::DestroyEditor(m_pNodeContext);
}

//using namespace ax;
HRESULT CNodeEditor::Initialize()
{
	ax::NodeEditor::Config config;
	config.SettingsFile = nullptr;

	m_pNodeContext = ax::NodeEditor::CreateEditor(&config);

	if (nullptr == m_pNodeContext)
		return E_FAIL;

	//노드
	m_Nodes.push_back(GUINODE(BEHAVIOR::SECQUNCE,0, "MainTex", _float2(40, 50), 0.5f, _float4(255, 100, 100, 1) ));
	m_Nodes.push_back(GUINODE(BEHAVIOR::SECQUNCE,1, "BumpMap", _float2(40, 150), 0.42f, _float4(200, 100, 100, 1) ));
	m_Nodes.push_back(GUINODE(BEHAVIOR::SELECTOR,2, "Combine", _float2(270, 80), 1.0f, _float4(0, 200, 100, 1)));

	m_NodesLink.push_back(GUINODE_LINK(2));
	m_NodesLink.push_back(GUINODE_LINK(2));

	return S_OK;
}
void CNodeEditor::UpdateGUI()
{
	
}
void CNodeEditor::RenderGUI()
{
}

void CNodeEditor::NodeEditorUpdate()
{
	if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(m_hTarget))
	{
		if (auto pComBt = pObj->GetComponent<CComBeHavior>("Com_BT"))
		{
			m_pBeHavior = pComBt;
			m_BTNodesMain = pComBt->Get_Selector()->Get_Nodes();

			Show_Editor();
		}
		else
		{
			m_pBeHavior = nullptr;
			m_BTNodesMain = nullptr;
		}
	}
}

HRESULT CNodeEditor::OpenBeHavior(CHandle Handle)
{
	if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(Handle))
	{
		if (auto pComBt = pObj->GetComponent<CComBeHavior>("Com_BT"))
		{
			m_hTarget = Handle;
			return S_OK;
		}
	}

	return E_FAIL;
}

void CNodeEditor::Show_Editor()
{
	if (nullptr == m_pBeHavior) return;

	ImGui::Begin("BeHavior Tree");
	// 구해줘
	ImGuiIO& io = ImGui::GetIO();
	_bool	bOpen_Context_Menu = false;//우클릭시 컨텍스트 메뉴롤 열도록 하는거
	int32_t iNode_hovered_in_list = -1; //현재 마우스에 올라간 노드 id hovered된(마우스가 어떤 요소위에 올라가있다) NodeId를 저장하는 변수
	int32_t iNode_hovered_in_scene = -1; //오른쪽 캔버스 공간(실제로 노드를 배치하는곳)에서 노드에 마우스 올라간곳 판정
	const _float fNode_Slot_Radius = 4.0f; //슬롯 반지름
	const _float2 fNode_Window_Padding(8.f, 8.f); //노드 내부 여백

	//왼쪽 패널용도
	NodeList_Panel(&iNode_hovered_in_list,&bOpen_Context_Menu);
	ImGui::BeginGroup(); //위젯 여러개를 묶는용도
	//오른쪽 캔버스창
	Begin_Canvas();
	//화면 격자
	Draw_Grid();

	//링크
	m_pDrawList->ChannelsSplit(2); //Draw List를 두개의 레이어로 나눈다?							   
	
	Draw_Link(); // 0번지에 링크 그리고

	//1번지에 노드 그려서
	Draw_Node(iNode_hovered_in_list, iNode_hovered_in_scene,fNode_Slot_Radius,fNode_Window_Padding,bOpen_Context_Menu, io,m_pBeHavior->Get_Selector());
	Draw_TmpNode(iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io);
	////위 2개를 합쳐라
	m_pDrawList->ChannelsMerge();
	

	//context menu 열기전 조건 검사
	//1. 오른쪽 버튼을 뗐다 ISMouseRelease Right
	//2. 현재 창 위에 마우스가 있다 Hovered
	//3. 또는 아무 아이템도 hover도 아니다
	//4. 열어라

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) || !ImGui::IsAnyItemHovered())
		{
			m_iNodeSelect = iNode_hovered_in_list = iNode_hovered_in_scene = -1;
			bOpen_Context_Menu = true;
		}
	}
		
	if (bOpen_Context_Menu)
	{
		ImGui::OpenPopup("context_menu");
		if (iNode_hovered_in_list != -1)
			m_iNodeSelect = iNode_hovered_in_list;

		if (iNode_hovered_in_scene != -1)
			m_iNodeSelect = iNode_hovered_in_scene;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	if (ImGui::BeginPopup("context_menu"))
	{
		GUINODE* pNode = m_iNodeSelect != -1 ? &m_Nodes[m_iNodeSelect] : NULL;
		if (pNode)
		{
			//노드에서 우클릭 -> Rename delete copy ...
			ImGui::Text("Node %s", pNode->Name);
			ImGui::Separator();
			if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
			if (ImGui::MenuItem("Delete..", NULL, false, false)) {}
			if (ImGui::MenuItem("Copy", NULL, false, false)) {}
		}
		else
		{
			//빈공간 우클릭...
			if (ImGui::MenuItem("Add_Selector"))
			{
				m_eBTType = BEHAVIOR::SELECTOR;
				m_pNodeName = "Selector";
				m_bPopup = true;
			}
			else if (ImGui::MenuItem("Add_Sequence"))
			{
				m_eBTType = BEHAVIOR::SECQUNCE;
				m_pNodeName = "Sequence";
				m_bPopup = true;
			}
			
			if (ImGui::MenuItem("Paste", NULL, false, false)) {}
		}


		ImGui::EndPopup();
	}
	ImVec2 vScene_Pos = ImGui::GetMousePosOnOpeningCurrentPopup() - ImVec2(m_vOffset.x, m_vOffset.y);
	if (m_bPopup)
		Add_Node(m_eBTType, m_pNodeName, vScene_Pos);

	ImGui::PopStyleVar();

	//휠로 캔버스 이동
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
	{
		m_vScroll.x += io.MouseDelta.x;
		m_vScroll.y += io.MouseDelta.y;
	}
		
	End_Canvas();
}
void CNodeEditor::NodeList_Panel(int32_t* piNode_hoverd_List, _bool* pbContext_Manu)
{
	ImGui::BeginChild("NodeList", ImVec2(100, 0)); //노드 리스트 ui 왼쪽 패널
	ImGui::Text("Nodes");
	ImGui::Separator();

	for (size_t i = 0; i < m_Nodes.size(); ++i)
	{
		GUINODE* node = &m_Nodes[i];
		ImGui::PushID(node->iID);
		if (ImGui::Selectable(node->Name.c_str(), node->iID == m_iNodeSelect))
			m_iNodeSelect = node->iID;
		if (ImGui::IsItemHovered()) //그 ui위에 마우스가 올라가있냐?
		{
			*piNode_hoverd_List = node->iID; //그럼 저장하라고~~
			*pbContext_Manu |= ImGui::IsMouseClicked(1);
		}
		ImGui::PopID();
	}

	ImGui::EndChild(); ImGui::SameLine();
}
void CNodeEditor::Begin_Canvas()
{
	ImGui::Text("Hold middle mouse button to scroll (%2.f,%2.f)", m_vScroll.x, m_vScroll.y);
	ImGui::SameLine(ImGui::GetWindowWidth() - 100);
	ImGui::Checkbox("Show grid", &m_bShow_grid);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
	ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PopStyleVar();
	ImGui::PushItemWidth(120.f);

	_float2 fCursorScrrenPos = _float2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
	XMStoreFloat2(&m_vOffset, XMLoadFloat2(&fCursorScrrenPos) + XMLoadFloat2(&m_vScroll));
	m_pDrawList = ImGui::GetWindowDrawList(); //이거하면 선 원 사각형 직접 그릴수있다네

}
void CNodeEditor::Draw_Grid()
{
	if (m_bShow_grid)
	{ // 격자를 그려라라라
		ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
		_float GRID_SZ = 64.f;
		ImVec2 vWin_pos = ImGui::GetCursorScreenPos(); //화면 좌표 시작점
		ImVec2 vCanvas_sz = ImGui::GetWindowSize();
		for (_float x = fmodf(m_vScroll.x, GRID_SZ); x < vCanvas_sz.x; x += GRID_SZ)
			m_pDrawList->AddLine(ImVec2(x, 0.f) + vWin_pos, ImVec2(x, vCanvas_sz.y) + vWin_pos, GRID_COLOR);
		for (_float y = fmodf(m_vScroll.y, GRID_SZ); y < vCanvas_sz.y; y += GRID_SZ)
			m_pDrawList->AddLine(ImVec2(0.0f, y) + vWin_pos, ImVec2(vCanvas_sz.x, y) + vWin_pos, GRID_COLOR);
	}
}
void CNodeEditor::Draw_Link()
{
	m_pDrawList->ChannelsSetCurrent(0); // layer0번지에 링크를 그리고
	//아 
	Recursive_Call_Node(m_pBeHavior->Get_Selector());
	

	for (size_t i = 0; i < m_BTNodesTmp.size(); ++i)
	{
		Recursive_Call_Node((m_BTNodesTmp[i]).get());
	}

	if (nullptr != m_CurrentNode.pCurrentNode)
	{
		Draw_NodeLine(m_CurrentNode.vSlotPos, _float2(ImGui::GetMousePos().x, ImGui::GetMousePos().y),true);
	}
	if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		Reset_CurrentNode();
}
void CNodeEditor::Draw_Node(int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io,  CBTRoot* pCurNode)
{
	GUINODE* pNode = &pCurNode->Get_GuiNodeInfo();
	GUINODE_LINK* pLink = &pCurNode->Get_GuiNodeLink();
	ImGui::PushID(pNode->iID); // 노드 내부에서 생성되는 widget id 중복방지용
	Widget(pNode, pLink, iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io);
	int32_t iSlot = Choice_StartSlot(pNode,fNode_Slot_Radius);

	if (-1 != iSlot)
	{
		GUICURRENT_NODE CurNode(pNode, &pCurNode->Get_GuiNodeLink(), iSlot);
		//그 위에있냐?
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			//시작 기준으로 잡기 
			if (-1 != pLink->iStartIdx && !m_CurrentNode.bSelected)
			{
				if (auto pParentNode = m_pBeHavior->Get_Node(pCurNode->Get_GuiNodeLink().SlotStart.DestName))
				{
					int32_t iParentIndex = pCurNode->Get_GuiNodeLink().SlotStart.iDestNode;
					pParentNode->Get_GuiNodeLink().SlotEnd[iParentIndex].Reset(); //그 부모도 시작지점 연결 끊기
					pCurNode->Get_GuiNodeLink().SlotStart.Reset(); //그 시작점이랑 연결된 원래 부모 연결끊기

					CurNode.eType = NODETYPE::START;
					CurNode.vSlotPos = pNode->GetStartSlotPos();
					CurNode.bSelected = true;
					m_CurrentNode = CurNode;
					m_CurrentNode.iD = m_BTNodesTmp.size();
					if (pParentNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR)
					{
						auto& pSrc = (*static_cast<CBTSelector*>(pParentNode)->Get_Nodes())[iParentIndex];
						m_BTNodesTmp.push_back(std::move(pSrc));
					}
					else if (pParentNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE)
					{
						auto& pSrc = (*static_cast<CBTSecqunce*>(pParentNode)->Get_Nodes())[iParentIndex];
						m_BTNodesTmp.push_back(std::move(pSrc));
					}
				}
			}
			//else if (m_CurrentNode.bSelected && m_CurrentNode.eType == NODETYPE::START)
			//{
			//	//노드 부모 END기준으로 Src Start에 연결
			//	pCurNode->Get_GuiNodeLink().SlotStart.PraentNode = m_CurrentNode.pCurrentNode->Get_DestInfo();
			//	m_CurrentNode.pCurrentLink->SlotEnd.ChildNode[iSlot] = pCurNode->Get_GuiNodeInfo().Get_DestInfo();
			//
			//	auto& pSrc = m_BTNodesTmp[m_CurrentNode.iD];
			//	
			//	Reset_CurrentNode();
			//}
		}
	}
	//출력이든 입력이든 해당 노드에서 선택 되는거 판정
	if (pNode->eMyType != BEHAVIOR::ACTION)
	{
		iSlot = Choice_EndSlot(pNode, pLink, fNode_Slot_Radius);
		if (-1 != iSlot)
		{
			GUICURRENT_NODE CurNode(pNode, &pCurNode->Get_GuiNodeLink(), iSlot);
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{//부모노드 기준에서 자식노드로 연결하는거
				//아 이거는 임시 저장소에 넣으면 안되네 이런
				if (-1 != pLink->SlotEnd[iSlot].iDestNode && !m_CurrentNode.bSelected)
				{	//부모기준으로 끊어도 동일하게 연결된 자식이 tmp로 빠지는걸로
					m_pParentTmp = pCurNode;
					if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR)
					{
						auto& pSrc = (*(static_cast<CBTSelector*>(pCurNode))->Get_Nodes())[iSlot];										
						pSrc->Get_GuiNodeLink().SlotStart.Reset();				//자식 기준 부모 끊기
						m_BTNodesTmp.push_back(std::move(pSrc));
					}
					else if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE)
					{
						auto& pSrc = (*(static_cast<CBTSecqunce*>(pCurNode))->Get_Nodes())[iSlot];
						pSrc->Get_GuiNodeLink().SlotStart.Reset();
						m_BTNodesTmp.push_back(std::move(pSrc));

					}
					pCurNode->Get_GuiNodeLink().SlotEnd[iSlot].Reset(); // 연결된 자식 끊기
	
					CurNode.eType = NODETYPE::NODE_END; //현재 선택한거
					CurNode.vSlotPos = pNode->GetEndSlotPos(iSlot,pLink->SlotEnd.size());
					CurNode.bSelected = true;
					m_CurrentNode.pCurrentLink->iStartIdx = iSlot;
					m_CurrentNode = CurNode;
					m_CurrentNode.iD = CurNode.iD;
					
				}
				else if (!m_CurrentNode.bSelected && m_CurrentNode.eType == NODETYPE::START)
				{
					if (pNode->eMyType == BEHAVIOR::SELECTOR)
					{

					}
					else if (pNode->eMyType == BEHAVIOR::SECQUNCE)
					{

					}
				}
			}
		}
	}
	if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR)
	{
		auto pSelector = static_cast<CBTSelector*>(pCurNode);
		for (size_t j = 0; j < pSelector->Get_Nodes()->size(); ++j)
		{
			if (nullptr == ((*pSelector->Get_Nodes())[j]))
				continue;
			
			Draw_Node(iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io, (*(pSelector->Get_Nodes()))[j].get());
		}
	}
	else if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE)
	{
		auto pSecqunce = static_cast<CBTSecqunce*>(pCurNode);
		for (size_t j = 0; j < pSecqunce->Get_Nodes()->size(); ++j)
		{
			if (nullptr == ((*pSecqunce->Get_Nodes())[j]))
				continue;

			Draw_Node(iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io, (*(pSecqunce->Get_Nodes()))[j].get());
		}
	}

	ImGui::PopID();



}

void CNodeEditor::Draw_TmpNode(int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io)
{
	for (auto iter = m_BTNodesTmp.begin(); iter != m_BTNodesTmp.end();)
	{
		if ((*iter)  == nullptr)
			continue;

		GUINODE* pNode = &(*iter)->Get_GuiNodeInfo(); //현재 그리려는 노드를 가져온다
		GUINODE_LINK* pLink = &(*iter)->Get_GuiNodeLink();
		ImGui::PushID(pNode->iID); // 노드 내부에서 생성되는 widget id 중복방지용

		Widget(pNode,pLink, iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io);
		//동그라미 위에있는지 
		//현재노드의 동글뱅이 거리랑 가깝냐?
		int32_t iSlot = Choice_StartSlot(pNode, fNode_Slot_Radius);
		if (-1 != iSlot)
		{
			GUICURRENT_NODE CurNode(pNode, &(*iter)->Get_GuiNodeLink(), iSlot);
			//그 위에있냐?
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && (*iter))
			{
				if (!Link_Connect_Check((*iter)->Get_GuiNodeLink().SlotStart.iDestNode) && !m_CurrentNode.bSelected) //현재 노드의 Start 부분에 연결된게 있는지?
				{
					CurNode.eType = NODETYPE::START;
					CurNode.vSlotPos = pNode->GetStartSlotPos();
					CurNode.bSelected = true;
					m_CurrentNode = CurNode;
					
				}
				else if (m_CurrentNode.bSelected && m_CurrentNode.eType == NODETYPE::NODE_END)
				{
					//여기서 원본 클래스 노드 부모에서 tmp 자식 에 연결하기
					(*iter)->Get_GuiNodeLink().SlotStart = m_pParentTmp->Get_GuiNodeInfo().Get_DestInfo(); // 자식에 부모 담았고
					int32_t iPreSlot = m_CurrentNode.pCurrentLink->iStartIdx;
					if (m_pParentTmp->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR)
					{
						
						auto pParn = static_cast<CBTSelector*>(m_pParentTmp);
						pParn->Get_GuiNodeLink().SlotEnd[iPreSlot] = (*iter)->Get_GuiNodeInfo().Get_DestInfo();
						(*pParn->Get_Nodes())[iPreSlot] = std::move(*iter);
						
					}
					else if (m_pParentTmp->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE)
					{
						auto pParn = static_cast<CBTSecqunce*>(m_pParentTmp);
						pParn->Get_GuiNodeLink().SlotEnd[iPreSlot] = (*iter)->Get_GuiNodeInfo().Get_DestInfo();

						(*pParn->Get_Nodes())[iPreSlot] = std::move(*iter);
					}

					Reset_CurrentNode();
					ImGui::PopID();
					return;
				}

			}
		}

		++iter;
		ImGui::PopID();
	}
}

int32_t CNodeEditor::Choice_EndSlot(GUINODE* pNode, GUINODE_LINK* pLink, const _float& fNode_Radius)
{
	//해당 노드 선택 됐을 경우에만진입
	for (uint32_t i = 0; i < pLink->SlotEnd.size(); ++i)
	{
		if (ImsMouseHoverSlot(pNode->GetEndSlotPos(i, pLink->SlotEnd.size()), fNode_Radius))
			return i;
	}
	return -1;
}
int32_t CNodeEditor::Choice_StartSlot(GUINODE* pNode, const _float& fNode_Radius)
{
	if (ImsMouseHoverSlot(pNode->GetStartSlotPos(), fNode_Radius))
			return 0;
	
	return -1;
}

void CNodeEditor::End_Canvas()
{//살려줘
	ImGui::PopItemWidth();
	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	ImGui::EndGroup();

	ImGui::End();
}

void CNodeEditor::Widget(GUINODE* pNode, GUINODE_LINK* pLink, int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io)
{
	_float2 vMin{};
	XMStoreFloat2(&vMin, XMLoadFloat2(&m_vOffset) + XMLoadFloat2(&pNode->vPos)); // 캔버스 기준 좌표를 스크린 좌표 기준으로 변환
	ImVec2 vNode_Rect_Min(vMin.x, vMin.y);

	m_pDrawList->ChannelsSetCurrent(1); //레이어 1에 UI 그려
	_bool	bOld_Any_Active = ImGui::IsAnyItemActive(); //위젯 생성전에 다른 위젯이 이미 활성화 되어있는지 저장
	ImGui::SetCursorScreenPos(vNode_Rect_Min + ImVec2(fNode_Window_Padding.x, fNode_Window_Padding.y)); //여기에 위젯을 만들어라
	//노드 내부으 imgui 위젯 생성	
	ImGui::BeginGroup();
	ImGui::Text("%s", pNode->Name.c_str()); //이름..
	ImGui::SliderFloat("##value", &pNode->fValue, 0.f, 1.f, "Alpha %2.f"); //알파값 뭐 쓸모가..
	ImGui::ColorEdit3("##Color", &pNode->vColor.x); //색상 뭐 쓸모가..
	ImGui::EndGroup();
	///////////////////위젯 생성

	_bool bNode_Widgets_active = (!bOld_Any_Active && ImGui::IsAnyItemActive());
	_float2 vDouble_Padding{};
	XMStoreFloat2(&vDouble_Padding, XMLoadFloat2(&fNode_Window_Padding) + XMLoadFloat2(&fNode_Window_Padding));
	ImVec2 vSize = ImGui::GetItemRectSize() + ImVec2(vDouble_Padding.x, vDouble_Padding.y);
	//Group 내부 Widget들의 크기를 자동 계산
	//GetItemRectSize는 gui가 text slider를 배치 후 가로 세로를 자동으로 계산해서 늘려준다네
	pNode->vSize = _float2(vSize.x, vSize.y);

	ImVec2 vNode_Rect_Max = vNode_Rect_Min + vSize;

	//뭐 노드 박스?
	m_pDrawList->ChannelsSetCurrent(0);
	ImGui::SetCursorScreenPos(vNode_Rect_Min);
	ImGui::InvisibleButton("node", vSize); //보이지않는 버튼 Talon
	//노드 전체에 버튼을 깔아서 노드 움직일수 있게해줌
	if (ImGui::IsItemHovered())
	{//마우스가 위에있고 우클릭을 했다? 그럼 매뉴창을연다
		iNode_hovered_in_scene = pNode->iID;
		bOpen_Context_Menu |= ImGui::IsMouseClicked(1);
	}

	_bool bNode_Moving_Active = ImGui::IsItemActive(); //클릭중이냐?
	if (bNode_Widgets_active || bNode_Moving_Active)
		m_iNodeSelect = pNode->iID;
	if (bNode_Moving_Active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) //드래그중이냐?
		pNode->vPos = _float2(pNode->vPos.x + io.MouseDelta.x, pNode->vPos.y + io.MouseDelta.y);

	//선택하거나 마우스 위에 올렸을때 scene인가 hover 확인해서 색상으로 표기
	ImU32 Node_Bg_Color = (iNode_hovered_in_list == pNode->iID ||
		iNode_hovered_in_scene == pNode->iID || m_iNodeSelect == pNode->iID ? IM_COL32(75, 75, 75, 255) : IM_COL32(60, 60, 60, 255));

	//노드 의 외형을 직접 그리는거
	//배경
	m_pDrawList->AddRectFilled(vNode_Rect_Min, vNode_Rect_Max, Node_Bg_Color, 4.f);
	//테두리차두리두리두리두리
	m_pDrawList->AddRect(vNode_Rect_Min, vNode_Rect_Max, IM_COL32(100, 100, 100, 255), 4.f);
	//동글뱅이 입력노드 출력노드
	
	m_pDrawList->AddCircleFilled(ImVec2(m_vOffset.x + pNode->GetStartSlotPos().x, m_vOffset.y + pNode->GetStartSlotPos().y), fNode_Slot_Radius, IM_COL32(150, 150, 150, 150));
	
	if (!pLink->SlotEnd.empty())
	{
		for (uint32_t i = 0; i < pLink->SlotEnd.size(); ++i)
			m_pDrawList->AddCircleFilled(ImVec2(m_vOffset.x + pNode->GetEndSlotPos(i,pLink->SlotEnd.size()).x, m_vOffset.y + pNode->GetEndSlotPos(i, pLink->SlotEnd.size()).y), fNode_Slot_Radius, IM_COL32(150, 150, 150, 150));
	}

}

void CNodeEditor::Add_Node(BEHAVIOR eType, const _char* pPopupName, ImVec2 vPos)
{
	CBTRoot::BTROOT_DESC SequenceDesc;
	_string PopupID = _string(pPopupName) + " Popup";
	_string InputName = _string(pPopupName) + " Name :";
	_char Name[32]{};
	_char NameBuffer[32]{};
	UPtr<CBTRoot>  pNode{ nullptr };
	ImGui::OpenPopup(PopupID.c_str());

	if (ImGui::BeginPopup(PopupID.c_str(), ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(InputName.c_str());
		if (ImGui::InputText("##NodeName", NameBuffer, IM_ARRAYSIZE(NameBuffer))) //이름 입력
			m_AddNodeName = _string(pPopupName) + " " + NameBuffer;
		
		if (ImGui::Button("Add"))
		{
			int32_t iIndex = 0;
			m_bPopup = false;
			SequenceDesc.m_GuiNode = GUINODE(eType, m_pBeHavior->Get_NodeID()++, m_AddNodeName.c_str(), _float2(vPos.x, vPos.y), 0.5f, _float4(100, 100, 200, 255));
			SequenceDesc.m_GuiLink = (GUINODE_LINK(2));
			iIndex = m_pBeHavior->Check_AllNode(m_AddNodeName);
			if (-1 == iIndex)
			{
				MSG_BOX("Failed : Node Name Same");
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return;
			}

			if (eType == BEHAVIOR::SELECTOR)
				pNode = CBTSelector::Create(&SequenceDesc);
			else if(eType == BEHAVIOR::SECQUNCE)
			pNode = CBTSecqunce::Create(&SequenceDesc);

			if (nullptr == pNode)
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return;
			}
				
			m_BTNodesTmp.push_back(std::move(pNode));
			ImGui::CloseCurrentPopup();
		}
		
		if (ImGui::Button("Cancle"))
		{
			m_bPopup = false;
			ImGui::CloseCurrentPopup();
		}
			
		ImGui::EndPopup();
	}

	//m_BTNodes[ETOUI(BEHAVIOR::SECQUNCE)].push_back());
}

_bool CNodeEditor::Link_Connect_Check(int32_t iSlot)
{
	//현재 노드 링크에 담겨있는거 start기준(노드의 id)으로 end랑 연결되게 되어있고
	//이거 nodelink 기준으로 스타트에 담겨있는거를 다시 누르려고하면 떨어지게 해야댐
	//만약 노드의 id가 nodeslink에 안담겨 있으면 해당 노드의 스타트 지점으로 부터 나가는 선은 없음 //이경우는 false지
	//근데 end 지점은또 연결 되어있을수도 있음
	if ( - 1 != iSlot)
		return true;
	return false;
}



void CNodeEditor::Reset_CurrentNode()
{
	m_CurrentNode.eType = NODETYPE::END;
	m_CurrentNode.pCurrentNode = nullptr;
	m_CurrentNode.pCurrentLink = nullptr;
	m_CurrentNode.vSlotPos = _float2(0,0);
	m_CurrentNode.iSelectedSlot = -1;
	m_CurrentNode.bSelected = false;
	
	m_pParentTmp = nullptr;
}
void CNodeEditor::Draw_NodeLine(_float2 iStartnode, _float2 iEndNode, _bool bMouse)
{
	_float2 p1{}, p2{}, input{}, output{};
	input = iStartnode; //동글뱅이 좌표
	output = iEndNode;
	XMStoreFloat2(&p1, XMLoadFloat2(&m_vOffset) +   //캔버스 화면 좌표에 내 좌표 더해서 화면 좌표로
		XMLoadFloat2(&input));

	if (bMouse)
		p2 = iEndNode;
	else
	{
		XMStoreFloat2(&p2, XMLoadFloat2(&m_vOffset) +
			XMLoadFloat2(&output));
	}
	//선 그리라고
	m_pDrawList->AddBezierCubic(ImVec2(p1.x, p1.y), ImVec2(p1.x, p1.y) + ImVec2(+50, 0),
		ImVec2(p2.x, p2.y) + ImVec2(-50, 0), ImVec2(p2.x, p2.y), IM_COL32(200, 200, 100, 255), 3.f);
}
_bool CNodeEditor::ImsMouseHoverSlot(_float2 vSlotPos, const _float& fNode_Radius)
{
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 vMousePos = io.MousePos;
	_vector vScreen{}, vMouse{ vMousePos.x, vMousePos .y,0,1.f};
	vScreen = XMLoadFloat2(&m_vOffset) + XMLoadFloat2(&vSlotPos);
	
	_float fDist = XMVectorGetX(XMVector2LengthSq(vScreen - vMouse));
	_bool  bhovered = fDist <= fNode_Radius * fNode_Radius;
	if (bhovered)
		int32_t i = 0;
	return bhovered;
}
void CNodeEditor::Recursive_Call_Node(class CBTRoot* pParent)
{
	GUINODE_LINK* pLink = &pParent->Get_GuiNodeLink();
	GUINODE* pNode_inp{ nullptr };
	GUINODE* pNode_out{ nullptr };
	BEHAVIOR eType = pParent->Get_GuiNodeInfo().eMyType;
	if (BEHAVIOR::SELECTOR == eType)
	{
		auto pSelector = static_cast<CBTSelector*>(pParent);
		
		for (size_t i = 0; i < (*pSelector->Get_Nodes()).size(); ++i)
		{
			if(-1 == (*pSelector->Get_Nodes())[i]->Get_GuiNodeLink().SlotEnd[i].iDestNode);
			continue;
			int32_t iParentIndex = (*pSelector->Get_Nodes())[i]->Get_GuiNodeLink().iStartIdx;

			pNode_inp = &(*pSelector->Get_Nodes())[i]->Get_GuiNodeInfo();
			pNode_out = &pParent->Get_GuiNodeInfo();
			Draw_NodeLine(pNode_inp->GetStartSlotPos(), pNode_out->GetEndSlotPos(iParentIndex, pParent->Get_GuiNodeLink().SlotEnd.size()));


			if ((*pSelector->Get_Nodes())[i]->Get_GuiNodeInfo().eMyType != BEHAVIOR::ACTION)
			Recursive_Call_Node(pSelector);
		}
	}
	else if (BEHAVIOR::SECQUNCE == eType)
	{
		auto pSequence = static_cast<CBTSecqunce*>(pParent);
		for (size_t i = 0; i < (*pSequence->Get_Nodes()).size(); ++i)
		{
			if (-1 == (*pSequence->Get_Nodes())[i]->Get_GuiNodeLink().SlotEnd[i].iDestNode)
			continue;

			int32_t iParentIndex = (*pSequence->Get_Nodes())[i]->Get_GuiNodeLink().iStartIdx;
			pNode_inp = &(*pSequence->Get_Nodes())[i]->Get_GuiNodeInfo();
			pNode_out = &pParent->Get_GuiNodeInfo();
			Draw_NodeLine(pNode_inp->GetStartSlotPos(), pNode_out->GetEndSlotPos(iParentIndex, pParent->Get_GuiNodeLink().SlotEnd.size()));


			if ((*pSequence->Get_Nodes())[i]->Get_GuiNodeInfo().eMyType != BEHAVIOR::ACTION)
			Recursive_Call_Node(pSequence);
		}
	}
}


UPtr<CNodeEditor> CNodeEditor::Create()
{
	auto pInstance = ToUPtr(new CNodeEditor());
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Create Failed : NodeEditor");
		return nullptr;
	}
	return pInstance;

}