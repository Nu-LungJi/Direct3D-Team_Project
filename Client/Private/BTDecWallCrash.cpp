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
	m_fTick += fTimeDelta;

	if(m_fTime >= m_fTick)
		return m_eDebug = EVALUATE::FAILED;

	if (!m_bCrash)
		m_bCrash = Wall_Crash();

	if(!m_bCrash)
		return m_eDebug = EVALUATE::FAILED;

	return m_eDebug = __super::Evaluate(fTimeDelta);
}
void CBTDecWallCrash::Update_Gui()
{
	ImGui::Text("Time : "); ImGui::SameLine();
	ImGui::DragFloat("##Time", &m_fTime, 0.1f, 0.f, 100.f);
}
nlohmann::json CBTDecWallCrash::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "Time",m_fTime);
	return  j;
}
HRESULT CBTDecWallCrash::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "Time", m_fTime);
	return S_OK;
}
void CBTDecWallCrash::Abort()
{
	__super::Abort();
	m_bCrash = false;
	m_fTick = 0.f;
}
void CBTDecWallCrash::OnEnter()
{
	m_fTick = 0.f;
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
	SweepDesc.tGeometry.fRadius = 4.f;
	SweepDesc.tPose.vPosition = vPos;
	SweepDesc.fMaxDistance = 4.f;
	XMStoreFloat4(&SweepDesc.tPose.vRotation, vLook);

	SweepDesc.tFilter = { .iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC_WALL),
	.hIgnoreGameObject = pOwner->GetHandle(),.bQueryStatic = true,.bQueryDynamic = false,.bIncludeTrigger =false };
	auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

	const auto vPreviousColor = pDbgLineRender->GetColor();
	const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
	pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
	pDbgLineRender->SetDepthTest(true);
	pDbgLineRender->AddSphere(SweepDesc.tGeometry.fRadius, XMMatrixTranslation(vPos.x, vPos.y, vPos.z));
	pDbgLineRender->SetColor(vPreviousColor);
	pDbgLineRender->SetDepthMode(ePreviousDepthMode);

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
