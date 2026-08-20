#include "pch.h"
#include "BTNaviMove.h"
#include "ComTransform.h" 
#include "ComCharacterMoveIntent.h"
#include "NpcMom.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
#include "NavMeshManager.h"
NS_USING(Client)

CBTNaviMove::CBTNaviMove()
{

}
CBTNaviMove::CBTNaviMove(const CBTNaviMove& rhs) : CBTActionNode(rhs)
{

}

CBTNaviMove::~CBTNaviMove()
{
}
HRESULT CBTNaviMove::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTNaviMove";
	return S_OK;
}
HRESULT CBTNaviMove::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

nlohmann::json CBTNaviMove::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonEnum(j, "MOVE", m_eMove);

	return j;
}

HRESULT CBTNaviMove::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonEnum(j, "MOVE", m_eMove);
	return S_OK;
}

EVALUATE CBTNaviMove::Evaluate(_float fTimeDelta)
{

	auto* pBT = Get_ComBT();
	if (nullptr == pBT) return m_eDebug = EVALUATE::FAILED;

	auto* pOwner = pBT->GetGameObject();

	auto* pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle,"ComCharacterMoveIntent");
	if (nullptr == pOwner || nullptr == pMoveIntent || m_NaviPath.empty())
		return m_eDebug = EVALUATE::FAILED;

	const _float3 vCurrentPosition = pOwner->GetTransform().GetPosition();

	// 현재 위치와 가까운 경로점은 다음 점으로 넘긴다.
	while (m_iNaviPathIndex < m_NaviPath.size())
	{
		_vector vToPathPoint = XMLoadFloat3(&m_NaviPath[m_iNaviPathIndex]) -
			XMLoadFloat3(&vCurrentPosition);

		vToPathPoint =	XMVectorSetY(vToPathPoint,	0.f);
		const _float fDistance = XMVectorGetX(XMVector3Length(vToPathPoint));

		if (fDistance > 0.7f)
			break;
		++m_iNaviPathIndex;
	}

	// 마지막 경로점에 도착
	if (m_iNaviPathIndex >= m_NaviPath.size())
	{
		// 다음 진입에는 반대편으로 이동
		m_bMoveToEnd =!m_bMoveToEnd;

		return m_eDebug = EVALUATE::SUCCESS;
	}

	_vector vMoveDirection =XMLoadFloat3(&m_NaviPath[m_iNaviPathIndex]) -
		XMLoadFloat3(&vCurrentPosition);

	vMoveDirection = XMVectorSetY(vMoveDirection,0.f);

	const _float fLengthSq =	XMVectorGetX(XMVector3LengthSq(vMoveDirection));

	if (fLengthSq <= FLT_EPSILON)
		return m_eDebug = EVALUATE::RUN;

	vMoveDirection =XMVector3Normalize(vMoveDirection);
	_float3 vDirection{};

	XMStoreFloat3(&vDirection,vMoveDirection);

	pMoveIntent->SetMoveIntent(vDirection,m_Value.fSpeed);

	pMoveIntent->SetFacingIntent(vDirection,360.f);

	return m_eDebug = EVALUATE::RUN;
}
void CBTNaviMove::Update_Gui()
{
	ImGui::Text("Speed");
	ImGui::DragFloat("##Speed", &m_Value.fSpeed, 0.1f, 0.f, 100.f);
}
void CBTNaviMove::Abort()
{
	__super::Abort();
	m_NaviPath.clear();
	m_iNaviPathIndex = 0;
}
void CBTNaviMove::OnEnter()
{
	__super::OnEnter();

	m_NaviPath.clear();
	m_iNaviPathIndex = 0;

	auto* pBT = Get_ComBT();
	if (nullptr == pBT)	return;

	auto* pBB = pBT->Get_Blackboard();
	auto* pOwner = pBT->GetGameObject();

	if (nullptr == pBB || nullptr == pOwner) return;
	auto* pvStart = pBB->Get_Value<_float3>( NPC_KEY::STARTPOS);
	auto* pvEnd = pBB->Get_Value<_float3>( NPC_KEY::ENDPOS);

	if (nullptr == pvStart || nullptr == pvEnd) 	return;

	auto* pNavMesh = CGameInstance::Get().GetNavMeshManager();
	if (nullptr == pNavMesh) return;

	// 시작 위치는 지정된 Start가 아니라 현재 NPC 위치를 사용
	const _float3 vCurrentPosition =
		pOwner->GetTransform().GetPosition();

	const _float3& vDestination = m_bMoveToEnd ? *pvEnd : *pvStart;

	pNavMesh->FindPath(vCurrentPosition, vDestination, m_NaviPath);
}
void CBTNaviMove::OnExit(EVALUATE eResult)
{
	__super::OnExit(eResult);

	m_NaviPath.clear();
	m_iNaviPathIndex = 0;
}
E::UPtr<CBTNaviMove> CBTNaviMove::Create()
{
	auto pInstance = E::ToUPtr(new CBTNaviMove{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTNaviMove");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTNaviMove::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTNaviMove{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTNaviMove");
		return nullptr;
	}

	return pInstance;
}
