#include "pch.h"
#include "BTDecHit.h"
#include "ComBeHavior.h"
NS_USING(Client)

CBTDecHit::CBTDecHit()
{

}
CBTDecHit::CBTDecHit(const CBTDecHit& rhs) : CBTDecorator(rhs)
{

}

CBTDecHit::~CBTDecHit()
{
}
HRESULT CBTDecHit::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecHit";
	return S_OK;
}
HRESULT CBTDecHit::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

nlohmann::json CBTDecHit::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	//SaveJsonEnum(j, "MOVE", m_eMove);

	return j;
}

HRESULT CBTDecHit::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	//LoadJsonEnum(j, "MOVE", m_eMove);
	return S_OK;
}

EVALUATE CBTDecHit::Evaluate(_float fTimeDelta)
{
	if (auto pCam = CGameInstance::Get().GetActiveCamera())
	{
		const auto& [vOri, vDir] = pCam->GetRay();
		_float fDist{};
		if (auto pVec = CGameInstance::Get().GetColliderGroup("CollTestGob"))
		{
			for (const auto& coll : *pVec)
			{
				if (coll->Intersect(vOri, vDir, fDist))
				{
					return __super::Evaluate(fTimeDelta);
				}
			}
		}
	}
	
	return m_eDebug = EVALUATE::FAILED;
}
void CBTDecHit::Update_Gui()
{
}
E::UPtr<CBTDecHit> CBTDecHit::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecHit{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecHit");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecHit::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecHit{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecHit");
		return nullptr;
	}

	return pInstance;
}
