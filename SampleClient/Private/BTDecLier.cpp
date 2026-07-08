#include "pch.h"
#include "BTDecLier.h" 
NS_USING(Client)

CBTDecLier::CBTDecLier()
{

}

CBTDecLier::CBTDecLier(const CBTDecLier& rhs) : CBTDecorator(rhs)
{

}
CBTDecLier::~CBTDecLier()
{
}
HRESULT CBTDecLier::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecLier";
	return S_OK;
}
HRESULT CBTDecLier::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecLier::Evaluate(_float fTimeDelta)
{
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	_float3 vDest = CGameInstance::Get().GetActiveCamera()->GetTransform().GetPosition();
	if (pTransform == nullptr)
		return EVALUATE::FAILED;
	_float3 vSrc = pTransform->GetPosition();
	_float fDistance = XMVectorGetX(XMVector3Length(XMLoadFloat3(&vSrc) - XMLoadFloat3(&vDest)));
	if (fDistance <= m_fDist)
	{
		return __super::Evaluate(fTimeDelta);
	}

	return EVALUATE::FAILED;
}
nlohmann::json CBTDecLier::Save_Node()
{
	nlohmann::json j;
	SaveJsonValue(j, "Distance", m_fDist);

	return j;
}
HRESULT CBTDecLier::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	if (!LoadJsonValue(j, "Distance", m_fDist))

		return S_OK;
}
void		CBTDecLier::Update_Gui()
{
	ImGui::Text("Distance %2.f : ");
	ImGui::DragFloat("##Dist", &m_fDist, 0, 100);
}
E::UPtr<CBTDecLier> CBTDecLier::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecLier{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecLier");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTDecLier::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecLier{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecLier");
		return nullptr;
	}

	return pInstance;
}
