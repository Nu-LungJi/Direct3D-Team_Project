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
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"

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

	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComPxRigidBody", &Desc, &m_pComRigidBody)))
		{
			MSG_BOX("Create Failed ComPxRigidBody Npc");
			return E_FAIL;
		}
	}

	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComRigidBody;
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.bIsTrigger = false;
		Desc.tFilter = PX_FILTER_DESC{
			.iLayer = ETOUI(COLLISION_LAYER::ENEMY_HURTBOX),
			.iSimulationMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
			//.iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
		};
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = 1.2f });
		if (!Desc.pResMaterial ||
			FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider,
				"ComPxSphereCollider", &Desc, &m_pComSphereCol)))
		{
			MSG_BOX("Create Failed ComPxSphereCollider Npc");
			return E_FAIL;
		}
		if (!m_pComSphereCol->SetQueryEnabled(false))
			return E_FAIL;
	}

	//피직스
	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		const _float fHorizontalScale =
			std::max(std::abs(NpcDesc->vScale.x), std::abs(NpcDesc->vScale.z));
		const _float fVerticalScale = std::abs(NpcDesc->vScale.y);
		const _float3 vCenterOffset{
			NpcDesc->vCCTCenterOffset.x * NpcDesc->vScale.x,
			NpcDesc->vCCTCenterOffset.y * fVerticalScale,
			NpcDesc->vCCTCenterOffset.z * NpcDesc->vScale.z };
		Desc.fHeight = NpcDesc->fCCTHeight * fVerticalScale;
		Desc.fRadius = NpcDesc->fCCTRadius * fHorizontalScale;
		Desc.fStepOffset = NpcDesc->fCCTStepOffset;
		Desc.vPosition = {
			NpcDesc->vPos.x + vCenterOffset.x,
			NpcDesc->vPos.y + vCenterOffset.y,
			NpcDesc->vPos.z + vCenterOffset.z };
		Desc.tFilter = NpcDesc->tFilter;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pCharacterController)))
		{
			return E_FAIL;
		}
	}
	//캐릭컨트롤러
	{
		CComCharacterMoveIntent::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
			"ComCharacterMoveIntent", &Desc, &m_pMoveIntent)))
		{
			return E_FAIL;
		}
	}
	//캐릭 모터
	{
		CComCharacterMotor::DESC Desc{};
		Desc.pMoveIntent = m_pMoveIntent;
		Desc.pCharacterController = m_pCharacterController;
		Desc.fGravity = -9.81f;
		Desc.vControllerCenterOffset = {
			NpcDesc->vCCTCenterOffset.x * NpcDesc->vScale.x,
			NpcDesc->vCCTCenterOffset.y * std::abs(NpcDesc->vScale.y),
			NpcDesc->vCCTCenterOffset.z * NpcDesc->vScale.z };
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor", &Desc, &m_pCharacterMotor)))
		{
			return E_FAIL;
		}
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



	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);
	GetTransform().SetPosition(XMLoadFloat3(&NpcDesc->vPos));

	m_pComSphereCol->SetQueryEnabled(true);
	m_pModelAnimator->Play_Anim(0, true);
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);

	m_vSpreadDir = _float3(Randf(-1.f, 1.f), Randf(-0.3f, 0.3f), Randf(-1.f, 1.f));
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
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CGriffChild::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);

}

void CGriffChild::Set_Neighbor(std::vector<CHandle>& Neighbors)
{
	for (auto& iter : Neighbors)
	{
		if (iter != GetHandle())
			m_Neighbors.push_back(iter);
	}
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
	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_TargetHandle);
	if (nullptr == pTarget) return;
	//부모보다 조금 뒤에 쫓아오게
	_vector vTargetPos = XMLoadFloat3(&pTarget->GetTransform().GetPosition());
	_vector vTargetLook = pTarget->GetTransform().GetState(STATE::LOOK);
	vTargetPos = vTargetPos - vTargetLook * 15.f;

	_vector vSrcPos = XMLoadFloat3(&GetTransform().GetPosition());
	_vector vSrcToTargetDir = XMVector3Normalize(vTargetPos - vSrcPos);
	_vector vMoveDir = XMVectorZero();
	_vector vAwaySum = XMVectorZero();
	
	_float fFarCheck = XMVectorGetX(XMVector3Length(vTargetPos - vSrcPos));

	_vector vFollowDir = XMVectorZero();
	_float fSpeed = 12.f;

	if (fFarCheck > 5.f)
	{//리더와 일정거리 떨어진 경우에만 따라오게
		_float fPower = (fFarCheck - 5.f) / 10.f;
		
		fPower = std::clamp(fPower, 0.f, 1.f);

		vFollowDir = vSrcToTargetDir * fPower;
		fSpeed = std::lerp(10.f, 15.f, fPower);
	}

	int32_t iCnt{};
	_float fRadius{ 30.f };
	_float fMinDis{ 25.f };
	_float fMinNeighbor{FLT_MAX};
	if (fFarCheck < 25.f)
	{
		for (auto& iter : m_Neighbors)
		{
			auto* pNeighbor = CGameInstance::Get().GetGameObjectByHandle(iter);
			if (nullptr == pNeighbor)continue;

			_vector vNeighborPos = XMLoadFloat3(&pNeighbor->GetTransform().GetPosition());
			_vector vAway = vSrcPos - vNeighborPos ;
			
			_float fDist = XMVectorGetX(XMVector3Length(vAway));

			if (fDist < fMinNeighbor)
				fMinNeighbor = fDist;
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
	}
	if(iCnt > 0)
	vAwaySum = vAwaySum / _float(iCnt);
	
	//if (fMinNeighbor < fMinDis)
	//	vMoveDir = vAwaySum;
	//else
		vMoveDir = vFollowDir * 1.5f + vAwaySum * 4.f;

	_vector vTargetDir = XMVector3Normalize(vMoveDir);
	_float fMoveLength = XMVectorGetX(XMVector3Length(vMoveDir));
	if (fMoveLength < 0.000001f)
	{
		m_pMoveIntent->SetMoveIntent({}, 0.f);
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

		XMStoreFloat3(&m_vCurDir, vLastDir);
	}

	m_pMoveIntent->SetFacingIntent(m_vCurDir, 60.f);
	m_pMoveIntent->SetMoveIntent(m_vCurDir, fSpeed);
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
