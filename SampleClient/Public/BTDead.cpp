#include "pch.h"
#include "BTDead.h" 
NS_USING(Client)

CBTDead::CBTDead()
{

}

CBTDead::CBTDead(const CBTDead& rhs) : CBTDecorator(rhs)
{

}
CBTDead::~CBTDead()
{
}
HRESULT CBTDead::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecSearch";
	return S_OK;
}
HRESULT CBTDead::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDead::Evaluate(_float fTimeDelta)
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
nlohmann::json CBTDead::Save_Node()
{
	nlohmann::json j;
	SaveJsonValue(j, "Distance", m_fDist);

	return j;
}
HRESULT CBTDead::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	if (!LoadJsonValue(j, "Distance", m_fDist))

		return S_OK;
}
void		CBTDead::Update_Gui()
{
	ImGui::Text("Distance %2.f : ");
	ImGui::DragFloat("##Dist", &m_fDist, 0, 100);
}
E::UPtr<CBTDead> CBTDead::Create()
{
	auto pInstance = E::ToUPtr(new CBTDead{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDead");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDead::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDead{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDead");
		return nullptr;
	}

	return pInstance;
}
