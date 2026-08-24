#include "pch.h"
#include "BTDecSearch.h" 
#include "Monster.h"
#include "NpcMom.h"
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
	SaveJsonEnum(j, "BTUserType", m_eUser);
	return j;
}

HRESULT CBTDecSearch::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "Distance", m_fDis);
	LoadJsonEnum(j, "BTUserType", m_eUser);
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

	CComTransform* pTargetTransform = nullptr;
	if (m_eUser == BT_USER::MON)
	{
		auto* pOwner = static_cast<CMonster*>(pBT->GetGameObject());
		if (pOwner)
		{
			if (auto pTarget = pOwner->Get_Target())
			{
				pTargetTransform = &pTarget->GetTransform();
			}
		}
			
	}
	else if (m_eUser == BT_USER::NPC)
	{
		auto* pOwner = static_cast<CNpcMom*>(pBT->GetGameObject());
		if (pOwner)
		{
			if (auto pTarget = pOwner->Get_Target())
			{
				pTargetTransform = &pTarget->GetTransform();
			}
		}
	}

	if(nullptr == pTargetTransform)
		return m_eDebug = EVALUATE::FAILED;
	auto& vSrc = pTransform;
	_vector vSrcPos = XMLoadFloat3(&vSrc->GetPosition());
	_vector vDestPos = XMLoadFloat3(&pTargetTransform->GetPosition());
	_vector vDeletYPos = XMVectorSetY(vSrcPos - vDestPos, 0.f);
	_float fDistance = XMVectorGetX(XMVector3Length(vDeletYPos));
	if (fDistance <= m_fDis)
		return  m_eDebug = __super::Evaluate(fTimeDelta);

	return  m_eDebug = EVALUATE::FAILED;
}

void		CBTDecSearch::Update_Gui()
{
	ImGui::DragFloat("##Dist", &m_fDis, 0, 100);
	if (ImGui::BeginCombo("User", MagicEnumToStringView(m_eUser).data()))
	{
		for (uint32_t i = 0; i < 2; ++i)
		{
			_bool bSelect = static_cast<BT_USER>(i) == m_eUser;

			if (ImGui::Selectable(MagicEnumToStringView(static_cast<BT_USER>(i)).data(),&bSelect))
			{
				m_eUser = static_cast<BT_USER>(i);
			}

			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
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
