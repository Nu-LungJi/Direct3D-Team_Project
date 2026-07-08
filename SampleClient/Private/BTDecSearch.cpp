#include "pch.h"
#include "BTDecSearch.h" 
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
HRESULT CBTDecSearch::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecSearch";
	return S_OK;
}
HRESULT CBTDecSearch::Initalize(void* pArg)
{
	
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecSearch::Evaluate(_float fTimeDelta)
{
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	if (pTransform == nullptr)
		return EVALUATE::FAILED;
	auto& vDest = CGameInstance::Get().GetActiveCamera()->GetTransform();
	auto& vSrc = pTransform;
	
	_vector vSrcPos = XMLoadFloat3(&vSrc->GetPosition());
	_vector vDestPos = XMLoadFloat3(&vDest.GetPosition());
	_float fDistance = XMVectorGetX(XMVector3Length(vSrcPos - vDestPos));
	if (fDistance <= m_fValue);
		return __super::Evaluate(fTimeDelta);
	

	return EVALUATE::FAILED;
}
nlohmann::json CBTDecSearch::Save_Node()
{
	nlohmann::json j;
	j = __super::Save_Node();
	SaveJsonValue(j, "Value", m_fValue);
	//SaveJsonValue(j, "UseDisAngle", m_bUseAngle);

	return j;
}
HRESULT CBTDecSearch::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	if (!LoadJsonValue(j, "Value", m_fValue))
		MSG_BOX("Failed Load Value : BTDecSearch");
	//if(!LoadJsonValue(j, "UseDisAngle", m_bUseAngle))
	//	MSG_BOX("Failed Save UseDisAngle : BTDecSearch");

	return S_OK;
}
void		CBTDecSearch::Update_Gui()
{
	ImGui::Text("Distance : %2.f");
	ImGui::DragFloat("##Dist", &m_fValue, 0, 100);
}
E::UPtr<CBTDecSearch> CBTDecSearch::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecSearch{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecSearch");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTDecSearch::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecSearch{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecSearch");
		return nullptr;
	}

	return pInstance;
}
