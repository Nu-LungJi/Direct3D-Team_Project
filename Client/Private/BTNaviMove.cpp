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
	SaveJsonValue(j, "Loop", m_bLoop);

	SaveJsonValue(j, "BBValue", m_bBBValue);
	return j;
}

HRESULT CBTNaviMove::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonEnum(j, "MOVE", m_eMove);
	LoadJsonValue(j, "Loop", m_bLoop);

	LoadJsonValue(j, "BBValue", m_bBBValue);
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

	_float3 vCurrentPos = pOwner->GetTransform().GetPosition();

	auto* pNavi = CGameInstance::Get().GetNavMeshManager();
	if (nullptr == pNavi)
		return m_eDebug = EVALUATE::FAILED;

	// 현재 위치와 가까운 경로점은 다음 점으로 넘긴다.
	
	_vector vToPathPoint = XMLoadFloat3(&m_NaviPath[m_iNaviPathIndex]) -
		XMLoadFloat3(&vCurrentPos);
	
	vToPathPoint =	XMVectorSetY(vToPathPoint,	0.f);
	_float fDistance = XMVectorGetX(XMVector3Length(vToPathPoint));
	if (fDistance <= 1.f)
	{
		if (m_iNaviPathIndex < m_NaviPath.size())
		++m_iNaviPathIndex;
	}
	
	// 마지막 경로점에 도착
	if (m_iNaviPathIndex >= m_NaviPath.size())
	{
		// 다음 진입에는 반대편으로 이동
		if(m_bLoop)
			m_bMoveToEnd = !m_bMoveToEnd;
		
		return m_eDebug = EVALUATE::SUCCESS;
	}

	//다음 지점으로 일직선 그엇을때 뭐 충돌하는거 없으면 현재 dir유지?
	//모르겠떵
	while (m_iNaviPathIndex + 1 < m_NaviPath.size())
	{
		const int32_t iNextIndex = m_iNaviPathIndex + 1;
		//한칸 뒤에 경로를 일직선으로 바로 갈수있는지 검사
		_vector vToNextPos = XMVectorSetY(XMLoadFloat3(&m_NaviPath[iNextIndex]) - 
			XMLoadFloat3(&vCurrentPos),0.f);

		_float fNextDis = XMVectorGetX(XMVector3Length(vToNextPos));
		
		if (fNextDis > 2.f)
			break;
		if (false == pNavi->NavMeshRayCast(vCurrentPos,m_NaviPath[iNextIndex]))
			break;
		m_iNaviPathIndex = iNextIndex;
	}
	_bool bInside = true;
	_float fPathRange = 0.8f;
	if (m_iNaviPathIndex > 0)
	{
		_vector vPathPos    = XMVectorSetY(XMLoadFloat3(&m_NaviPath[m_iNaviPathIndex - 1]),0.f);
		_vector vEndPathPos = XMVectorSetY(XMLoadFloat3(&m_NaviPath[m_iNaviPathIndex]),0.f);
		_vector vCurPos     = XMVectorSetY(XMLoadFloat3(&vCurrentPos),0.f);

		_vector vPathLine = vEndPathPos - vPathPos;
		_vector vFromStart = vCurPos - vPathPos;
		_float fPathLengthSq = XMVectorGetX(XMVector3LengthSq(vPathLine));
		if (fPathLengthSq > FLT_EPSILON)
		{
			_float fLineRatio = std::clamp(XMVectorGetX(XMVector3Dot(vFromStart, vPathLine)) / fPathLengthSq,0.f,1.f);
			_vector vNear = vPathPos + vPathLine * fLineRatio;
			_float fDisFromPathSq = XMVectorGetX(XMVector3LengthSq(vCurPos - vNear));

			bInside = fDisFromPathSq <= fPathRange * fPathRange;
		}
	}
	_vector vMoveDirection = XMVectorSetY(XMLoadFloat3(&m_NaviPath[m_iNaviPathIndex]) -
		XMLoadFloat3(&vCurrentPos), 0.f);

	_float fLengthSq = XMVectorGetX(XMVector3LengthSq(vMoveDirection));

	if (fLengthSq <= FLT_EPSILON)
		return m_eDebug = EVALUATE::RUN;

	vMoveDirection = XMVector3Normalize(vMoveDirection);

	_bool bSweep = Sweep(vMoveDirection,
		XMVector3Normalize(pOwner->GetTransform().GetState(STATE::LOOK)), vCurrentPos,
		std::max(1.f, m_Value.fSpeed * 0.4f));

	if (bSweep && m_bLoop)
	{
		//뭐 부딪히면 45도 돌리고
		_vector vAvoidDir = XMVector3Normalize(XMVector3TransformNormal(vMoveDirection, XMMatrixRotationY(
			XMConvertToRadians(45.f))));

		//돌린 위치로 미리 위치 일정거리 확인해서
		_float fCheckDist = std::max(1.f, m_Value.fSpeed * 0.4f);
		_vector vCheckPos = XMLoadFloat3(&vCurrentPos) + vAvoidDir * fCheckDist;

		_float3 vCheckTarget{};
		XMStoreFloat3(&vCheckTarget, vCheckPos);
		// 삼각형있는지 검사
		if (pNavi->NavMeshRayCast(vCurrentPos, vCheckTarget))
			vMoveDirection = vAvoidDir;
	}
	

	_vector vCurDir = XMLoadFloat3(&m_vLastDir);
	_vector vTargetDir = vMoveDirection;

	if (XMVectorGetX(XMVector3LengthSq(vCurDir)) <= FLT_EPSILON)
		vCurDir = vMoveDirection;
	
	_float fFoward = std::max(1.f, m_Value.fSpeed * 0.5f);
	_vector vFowardPos = XMLoadFloat3(&vCurrentPos) + vCurDir * fFoward;
	_float3 vFowardTarget{};
	XMStoreFloat3(&vFowardTarget, vFowardPos);
	_float fDirDot = XMVectorGetX(XMVector3Dot(vCurDir, vTargetDir));
	_bool bStraight = bInside && false == bSweep && fDirDot >= 0.8f && pNavi->NavMeshRayCast(vCurrentPos, vFowardTarget);

	_float fTurnRatio = std::min(1.f, m_Value.fSpeed * fTimeDelta * 2.f);
	if (bStraight)
		vMoveDirection = vCurDir;
	else
	{
		_vector vTurnDir = XMVector3Normalize(XMVectorLerp(vCurDir, vTargetDir, fTurnRatio));

		_float fMoveCheckDist = std::max(0.3f, m_Value.fSpeed * fTimeDelta * 2.f);
		_float3 vTurnTarget{};
		XMStoreFloat3(&vTurnTarget, XMLoadFloat3(&vCurrentPos) + vTurnDir * fMoveCheckDist);
		if (pNavi->NavMeshRayCast(vCurrentPos, vTurnTarget))
		{
			vMoveDirection = vTurnDir;
		}
		else
			vMoveDirection = vTargetDir;
	}
	XMStoreFloat3(&m_vLastDir, vMoveDirection);
	_float3 vDirection{};
	//외않됨
	XMStoreFloat3(&vDirection, vMoveDirection);
	pMoveIntent->SetMoveIntent(vDirection, m_Value.fSpeed);
	pMoveIntent->SetFacingIntent(vDirection, 45.f);

	return m_eDebug = EVALUATE::RUN;
}
void CBTNaviMove::Update_Gui()
{
	ImGui::Text("Speed");
	ImGui::DragFloat("##Speed", &m_Value.fSpeed, 0.1f, 0.f, 100.f);

	if (ImGui::Button(m_bLoop == true ? "Loop : TRUE" : "Loop : FALSE"))
		m_bLoop = !m_bLoop;

	if (ImGui::Button(m_bBBValue == true ? "BBValue : TRUE" : "BBValue : FALSE"))
		m_bBBValue = !m_bBBValue;
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

	auto* pNavMesh = CGameInstance::Get().GetNavMeshManager();
	if (nullptr == pNavMesh) return;

	// 시작 위치는 지정된 Start가 아니라 현재 NPC 위치를 사용
	_float3 vCurrentPosition = pOwner->GetTransform().GetPosition();
	
	
	if (m_bLoop)
	{
		auto* pvStart = pBB->Get_Value<_float3>(NPC_KEY::STARTPOS);
		auto* pvEnd = pBB->Get_Value<_float3>(NPC_KEY::ENDPOS);

		if (nullptr == pvStart || nullptr == pvEnd) 	return;
		_float3& vDestination = m_bMoveToEnd ? *pvEnd : *pvStart;
		pNavMesh->FindPathCenter(vCurrentPosition, vDestination, m_NaviPath);
	}
	else
	{
		auto* pBB = pBT->Get_Blackboard();
		if (pBB)
		{
			auto* pTargetHandle = pBB->Get_Value<CHandle>(PUBLIC_KEY::TARGETHANDLE);
			if (pTargetHandle)
			{
				auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(*pTargetHandle);
				if (pTarget)
				{
					auto& TargetTransform = pTarget->GetTransform();
					pNavMesh->FindPathCenter(vCurrentPosition, TargetTransform.GetPosition(), m_NaviPath);
				}
				
			}
			
		}
	}
		
	XMStoreFloat3(&m_vLastDir, pOwner->GetTransform().GetState(STATE::LOOK));
	BBValue(pBB);
}
void CBTNaviMove::OnExit(EVALUATE eResult)
{
	__super::OnExit(eResult);

	m_NaviPath.clear();
	m_iNaviPathIndex = 0;
}
void CBTNaviMove::BBValue(CBTBlackBoard* pBB)
{
	if (!m_bBBValue)
		return;

	auto* pSpeed = pBB->Get_Value<_float>(NPC_KEY::SPEED);
	if (nullptr == pSpeed) return;
	m_Value.fSpeed = *pSpeed;
}
_bool CBTNaviMove::Sweep(_vector vNextDir, _vector vCurDir, _float3 vCurPos, _float fDist)
{
	PX_SWEEP_DESC SweepDesc{};

	SweepDesc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	SweepDesc.tPose.vPosition = vCurPos;
	SweepDesc.tPose.vPosition.y += 0.8f;
	XMStoreFloat3(&SweepDesc.vDirection, vCurDir);
	SweepDesc.fMaxDistance = fDist; 
	SweepDesc.tFilter.iQueryMask = ETOUI(COLLISION_LAYER::DEFAULT) | ETOUI(COLLISION_LAYER::WORLD_STATIC) | ETOUI(COLLISION_LAYER::NPC_BODY);
	SweepDesc.tFilter.hIgnoreGameObject = m_Handle;
	SweepDesc.tFilter.bQueryStatic = true;
	SweepDesc.tFilter.bQueryDynamic = true;
	SweepDesc.tFilter.bIncludeTrigger = false;

	PX_SWEEP_RESULT Hit{};
	auto* pPhysX = CGameInstance::Get().GetPhysXManager();

	if (pPhysX && pPhysX->Sweep(SweepDesc, Hit) && Hit.bHit)
	{
		return true;
		
	}
	return false;
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
