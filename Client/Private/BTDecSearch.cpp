#include "pch.h"
#include "BTDecSearch.h" 
#include "Monster.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
NS_USING(Client)

CBTDecSearch::CBTDecSearch()
{

}

CBTDecSearch::CBTDecSearch(const CBTDecSearch& rhs) : CBTDecorator(rhs)
{

}
CBTDecSearch::~CBTDecSearch()
{
}
HRESULT CBTDecSearch::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecSearch";
	return S_OK;
}
HRESULT CBTDecSearch::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}

nlohmann::json CBTDecSearch::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "Distance", m_fDis);
	SaveJsonValue(j, "Run", m_bRunning);

	
	return j;
}

HRESULT CBTDecSearch::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "Distance", m_fDis);
	LoadJsonValue(j, "Run", m_bRunning);

	return S_OK;
}

EVALUATE CBTDecSearch::Evaluate(_float fTimeDelta)
{
	if (m_bTrue)
		return m_eDebug = EVALUATE::SUCCESS;
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	if (pTransform == nullptr)
		return m_eDebug = EVALUATE::FAILED;
	auto* pBT = Get_ComBT();
	if(nullptr == pBT) return m_eDebug = EVALUATE::FAILED;
	auto* pBB = pBT->Get_Blackboard();
	if(nullptr == pBB) return m_eDebug = EVALUATE::FAILED;

	auto* pTargetHandle = pBB->Get_Value<CHandle>(PUBLIC_KEY::TARGETHANDLE);
	if(nullptr == pTargetHandle) return m_eDebug = EVALUATE::FAILED;

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(*pTargetHandle);
	if(nullptr == pTarget) return m_eDebug = EVALUATE::FAILED;

	auto& vSrc = pTransform;
	_vector vSrcPos = XMLoadFloat3(&vSrc->GetPosition());
	_vector vDestPos = XMLoadFloat3(&pTarget->GetTransform().GetPosition());
	_vector vDeletYPos = XMVectorSetY(vSrcPos - vDestPos, 0.f);
	_float fDistance = XMVectorGetX(XMVector3Length(vDeletYPos));

	if(m_bRunning && m_PreEval == EVALUATE::RUN)
		return  m_eDebug = __super::Evaluate(fTimeDelta);

	if (fDistance <= m_fDis)
		return  m_PreEval = m_eDebug = __super::Evaluate(fTimeDelta);

	return  m_eDebug = EVALUATE::FAILED;
}

void		CBTDecSearch::Update_Gui()
{
	if (ImGui::Button(m_bRunning == true ? "RUN : TRUE" : "RUN : FALSE"))
		m_bRunning = !m_bRunning;

	ImGui::DragFloat("##Dist", &m_fDis, 0, 100);
}
void CBTDecSearch::Abort()
{
	__super::Abort();
	m_PreEval = EVALUATE::SUCCESS;
}
void CBTDecSearch::OnEnter()
{
	m_PreEval = EVALUATE::SUCCESS;
}
void CBTDecSearch::OnExit(EVALUATE eResult)
{
}
E::UPtr<CBTDecSearch> CBTDecSearch::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecSearch{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecSearch");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecSearch::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecSearch{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecSearch");
		return nullptr;
	}

	return pInstance;
}
