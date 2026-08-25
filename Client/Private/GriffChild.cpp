#include "pch.h"
#include "GriffChild.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "DbgLineRender.h"
#include "Griff.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
NS_USING(Client)

CGriffChild::CGriffChild()
{
}


CGriffChild::~CGriffChild()
{
}

void CGriffChild::UpdateGUI()
{
	__super::UpdateGUI();

}

HRESULT CGriffChild::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CGriffChild::Initialize(void* pArg)
{
	auto NpcDesc = static_cast<ANIMAL_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}

	CComBeHavior::BEHAVIOR_DESC Desc{};
	Desc.OwnerName = "Com_BT";
	Desc.resBeHaviorMajor = NpcDesc->resBeHaviorMajor;
	Desc.resBeHaviorMinor = NpcDesc->resBeHaviorMinor;
	if (FAILED(AddComponentFromProto("BEHAVIOR", "Prototype_Component_BeHavior", "Com_BT", &Desc, &m_pBeHavior)))
	{
		return E_FAIL;
	};
	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}
	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = NpcDesc->LevelTag;
		Desc.sResTag = NpcDesc->ReSourceTag;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	{
		CComAnimator::DESC DescAnim{};
		DescAnim.sComTag = "ComCModelIntance";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Animator", "ComCModelAnimator", &DescAnim, &m_pModelAnimator)))
		{
			return E_FAIL;
		};
	}

	{
		CComCollider::DESC Desc{};
		Desc.eCollType = CollType::Box;
		Desc.vExtents = { 1.f, 1.f, 1.f };
		if (FAILED(AddComponentFromProto("COLLIDER", "Prototype_Component_Collider", "ComColl", &Desc, &m_pComCollider)))
		{
			return E_FAIL;
		};
	}



//	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);
	GetTransform().SetPosition(XMLoadFloat3(&NpcDesc->vPos));

	m_pModelAnimator->Play_Anim(0, true);
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);

	m_vSpreadDir = _float3(Randf(-1.f, 1.f), Randf(-0.3f, 0.3f), Randf(-1.f, 1.f));
	m_vOffsetPos = _float3(Randf(-20.f, 20.f), Randf(-8.f, 8.f), Randf(0.f, 15.f));
	return S_OK;
}

void CGriffChild::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);
	Chase_Leader(fTimeDelta);

}

void CGriffChild::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CGriffChild::FixedUpdate(E::_float fTimeDelta)
{
	//m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CGriffChild::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);

}



void CGriffChild::Set_Gravity(_bool bGravity)
{
	if (bGravity)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	else
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);
}

void CGriffChild::Chase_Leader(_float fTimeDelta)
{
	auto* pTarget = CGameInstance::Get().GetGameObjectByHandleT<CGriff>(m_TargetHandle);
	if (nullptr == pTarget) return;
	//부모보다 조금 뒤에 쫓아오게
	_vector vTargetPos = XMLoadFloat3(&pTarget->GetTransform().GetPosition());
	_vector vTargetRight = XMVector3Normalize(pTarget->GetTransform().GetState(STATE::RIGHT));
	_vector vTargetUp    = XMVector3Normalize(pTarget->GetTransform().GetState(STATE::UP));
	_vector vTargetLook  = XMVector3Normalize(pTarget->GetTransform().GetState(STATE::LOOK));

	//뒤로 퍼지게
	vTargetPos = vTargetPos + vTargetRight * m_vOffsetPos.x
		+ vTargetUp * m_vOffsetPos.y
		- vTargetLook * (30.f + m_vOffsetPos.z);

	_vector vSrcPos = XMLoadFloat3(&GetTransform().GetPosition());
	_vector vSrcToTargetDir = XMVector3Normalize(vTargetPos - vSrcPos);
	_vector vMoveDir = XMVectorZero();
	_vector vAwaySum = XMVectorZero();
	
	_float fFarCheck = XMVectorGetX(XMVector3Length(vTargetPos - vSrcPos));
	
	if (fFarCheck <= 5.f)
	{
		//m_pMoveIntent->SetMoveIntent({}, 0.f);
		return;
	}
	_vector vFollowDir = XMVectorZero();
	//리더와 일정거리 떨어진 경우에만 따라오게
	_float fPower = (fFarCheck - 5.f) / 25.f;
	
	fPower = std::clamp(fPower, 0.f, 1.f);

	vFollowDir = vSrcToTargetDir * fPower;
	_float fSpeed = std::lerp(30.f, 33.f, fPower);
	

	int32_t iCnt{}, iAligCnt{}, iCohesionCnt{};
	_float fRadius{ 30.f };
	_float fMinNeighbor{FLT_MAX};
	_vector vDirAverage = XMVectorZero();
	_vector vCenterSum = XMVectorZero();
	_vector vCohesionDir = XMVectorZero();
	
	for (auto& iter : pTarget->Get_Neighbor())
	{
		if (iter == GetHandle())
			continue;
		auto* pNeighbor = CGameInstance::Get().GetGameObjectByHandle(iter);
		if (nullptr == pNeighbor)continue;

		_vector vNeighborPos = XMLoadFloat3(&pNeighbor->GetTransform().GetPosition());
		_vector vAway = vSrcPos - vNeighborPos ;
		
		_float fDist = XMVectorGetX(XMVector3Length(vAway));

		if (fDist < fMinNeighbor)
			fMinNeighbor = fDist;

		if (fDist < 80.f)
		{
			vCenterSum += vNeighborPos;
			++iCohesionCnt;
		}

		if (fDist < 20.f)
		{ //방향 평균
			_vector vNeighborDir = pNeighbor->GetTransform().GetState(STATE::LOOK);
			vDirAverage += XMVector3Normalize(vNeighborDir);
			++iAligCnt;
		}
		//내 이웃이랑 가까우면 밀어내기
		//너무 딱 붙으면 아에 다른 방향으로
		
		if (fDist <= 0.01f)
		{
			vAwaySum += XMLoadFloat3(&m_vSpreadDir);
			++iCnt;
			continue;
		}
		//밀어내 이웃이랑 근처면 이웃들 순회해서 누적한만큼
		if (fDist < fRadius)
		{
			vAwaySum += XMVector3Normalize(vAway) * (1.f - fDist / fRadius);
			++iCnt;
		}
	}
	
	if(iCnt > 0)
		vAwaySum = vAwaySum / _float(iCnt);
	if (iAligCnt > 0)
		vDirAverage = vDirAverage / _float(iAligCnt);
	if (iCohesionCnt > 0)
	{
		_vector vCenter = vCenterSum / _float(iCohesionCnt);
		_vector vToCenter = vCenter - vSrcPos;
		_float fCenterDist = XMVectorGetX(XMVector3Length(vToCenter));

		if (fCenterDist > 35.f)
		{
			_float fCohesionPower = (fCenterDist - 35.f) / 30.f;
			fCohesionPower = std::clamp(fCohesionPower, 0.f, 1.f);
			vCohesionDir = XMVector3Normalize(vToCenter) * fCohesionPower;
			//무리에서 이탈할경우 많이 벗어나면 돌아오게하기 위함
		}
	}


	vMoveDir = vTargetLook * 1.f + vFollowDir * 1.5f 
		+ vAwaySum * 2.f + vDirAverage * 0.6f + vCohesionDir * 0.3f;

	_vector vTargetDir = XMVector3Normalize(vMoveDir);
	_float fMoveLength = XMVectorGetX(XMVector3Length(vMoveDir));
	if (fMoveLength < 0.000001f)
	{
		//m_pMoveIntent->SetMoveIntent({}, 0.f);
		return;
	}
	_vector vCurDir = XMLoadFloat3(&m_vCurDir);
	if (XMVectorGetX(XMVector3Length(vCurDir)) < 0.00001f)
	{
		XMStoreFloat3(&m_vCurDir, vTargetDir);
	}
	else
	{
		_float fRatio = std::clamp(fTimeDelta * 3.f, 0.f, 1.f);

		_vector vLastDir = XMVectorLerp(
			XMVector3Normalize(XMLoadFloat3(&m_vCurDir)),vTargetDir,fRatio);

		XMStoreFloat3(&m_vCurDir, XMVector3Normalize(vLastDir));
	}


	_float fYaw = std::atan2(m_vCurDir.x, m_vCurDir.z);

	m_pComTransform->SetQuaternion(XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), fYaw));
	_float3 vPos{};
	XMStoreFloat3(&vPos, vSrcPos + XMLoadFloat3(&m_vCurDir) * fSpeed * fTimeDelta);
	m_pComTransform->SetPosition(vPos);
}

E::UPtr<CGriffChild> CGriffChild::Create()
{
	auto pInstance = E::ToUPtr(new CGriffChild{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CGriffChild");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CGriffChild::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CGriffChild{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CGriffChild");
		return nullptr;
	}

	return pInstance;
}
