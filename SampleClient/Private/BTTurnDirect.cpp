#include "pch.h"
#include "BTTurnDirect.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTTurnDirect::CBTTurnDirect()
{

}

CBTTurnDirect::CBTTurnDirect(const CBTTurnDirect& rhs) : CBTActionNode(rhs)
{

}
CBTTurnDirect::~CBTTurnDirect()
{
}
HRESULT CBTTurnDirect::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTTurnDirect";
	return S_OK;
}
HRESULT CBTTurnDirect::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTTurnDirect::Evaluate(_float fTimeDelta)
{
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto& vDest = CGameInstance::Get().GetActiveCamera()->GetTransform();
	if (pTransform == nullptr)
	{
		m_eDebug = EVALUATE::FAILED;
		return EVALUATE::FAILED;
	}
	XMMATRIX mat = XMMatrixIdentity();
	_vector vLook = XMVector3Normalize((vDest.GetState(STATE::POSITION) - pTransform->GetState(STATE::POSITION)));
	_vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), vLook));
	_vector vUp = XMVector3Cross(vLook, vRight);

	mat.r[0] = vRight;
	mat.r[1] = vUp;
	mat.r[2] = vLook;

	XMVECTOR quat = XMQuaternionRotationMatrix(mat);
	pTransform->SetQuaternion(quat);

	m_eDebug = EVALUATE::SUCCESS;
	return EVALUATE::SUCCESS;
}
void CBTTurnDirect::Update_Gui()
{
	ImGui::Text("TickTime %2.f : ");
	ImGui::DragFloat("##Tick", &m_Value.fTime, 0, 100);
}
E::UPtr<CBTTurnDirect> CBTTurnDirect::Create()
{
	auto pInstance = E::ToUPtr(new CBTTurnDirect{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTurnDirect");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTTurnDirect::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTurnDirect{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTurnDirect");
		return nullptr;
	}

	return pInstance;
}
