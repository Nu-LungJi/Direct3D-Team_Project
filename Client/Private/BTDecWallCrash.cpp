#include "pch.h"
#include "BTDecWallCrash.h"
#include "ComBeHavior.h"
#include "PhysXManager.h"
NS_USING(Client)

CBTDecWallCrash::CBTDecWallCrash()
{

}
CBTDecWallCrash::CBTDecWallCrash(const CBTDecWallCrash& rhs) : CBTDecorator(rhs)
{

}

CBTDecWallCrash::~CBTDecWallCrash()
{
}
HRESULT CBTDecWallCrash::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecWallCrash";
	return S_OK;
}
HRESULT CBTDecWallCrash::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecWallCrash::Evaluate(_float fTimeDelta)
{

	if (!m_bCrash)
		m_bCrash = Wall_Crash();

	if(!m_bCrash)
		return m_eDebug = EVALUATE::FAILED;

	return m_eDebug = __super::Evaluate(fTimeDelta);
}
void CBTDecWallCrash::Update_Gui()
{

}
nlohmann::json CBTDecWallCrash::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	return  j;
}
HRESULT CBTDecWallCrash::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	return S_OK;
}
void CBTDecWallCrash::Abort()
{
	__super::Abort();
	m_bCrash = false;
}
void CBTDecWallCrash::OnEnter()
{
	m_bCrash = false;
}
_bool CBTDecWallCrash::Wall_Crash()
{
	auto pBT = Get_ComBT();
	if (nullptr == pBT) return false;

	auto pOwner = pBT->GetGameObject();
	if (nullptr == pOwner) return false;

	_vector vLook = XMVector3Normalize(XMVectorSetY(pOwner->GetTransform().GetState(STATE::LOOK),0.f));
	_float3 vPos = pOwner->GetTransform().GetPosition();
	PX_SWEEP_DESC SweepDesc{};
	SweepDesc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	SweepDesc.tGeometry.fRadius = 0.5f;
	SweepDesc.tPose.vPosition = vPos;
	XMStoreFloat4(&SweepDesc.tPose.vRotation, vLook);

	SweepDesc.tFilter = { .iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC),
	.hIgnoreGameObject = pOwner->GetHandle(),.bQueryStatic = true,.bQueryDynamic = false,.bIncludeTrigger =false };

	PX_SWEEP_RESULT SweepResult{};

	auto* pPhysX = CGameInstance::Get().GetPhysXManager();
	if (nullptr == pPhysX) return false;

	return pPhysX->Sweep(SweepDesc, SweepResult) && SweepResult.bHit;
}
E::UPtr<CBTDecWallCrash> CBTDecWallCrash::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecWallCrash{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecWallCrash");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecWallCrash::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecWallCrash{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecWallCrash");
		return nullptr;
	}

	return pInstance;
}
