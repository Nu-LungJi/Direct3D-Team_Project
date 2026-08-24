#include "pch.h"
#include "Griff.h"
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
#include "GriffChild.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
NS_USING(Client)

CGriff::CGriff()
{
}


CGriff::~CGriff()
{
}

void CGriff::UpdateGUI()
{
	__super::UpdateGUI();

}

HRESULT CGriff::InitializePrototype(void* pArg)
{
	if (FAILED(__super::InitializePrototype(pArg)))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CGriff::Initialize(void* pArg)
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

	m_WayPoint.push_back(_float3(30.f, 75.f, -346.f));
	m_WayPoint.push_back(_float3(-8.f, 135.f, -61.f));
	m_WayPoint.push_back(_float3(213.f, 122.f, -82.f));
	m_WayPoint.push_back(_float3(360.f, 169.f, 19.f));
	m_WayPoint.push_back(_float3(292.f, 186.f, 215.f));
	m_WayPoint.push_back(_float3(-196.f, 139.f, 23.f));
	m_WayPoint.push_back(_float3(30.f, 75.f, -346.f));
	return S_OK;
}

void CGriff::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);
	if (m_WayPoint.empty())
		return;

	if (m_WayPoint.size() <= m_iIndex)
	{
		m_bLoop = false;
		--m_iIndex;
	}
	else if (m_iIndex <= 0)
	{
		m_bLoop = true;
	}

	_vector vNextPos = XMLoadFloat3(&m_WayPoint[m_iIndex]);
	_vector vCurPos = XMLoadFloat3(&GetTransform().GetPosition());

	_vector vLen = vNextPos - vCurPos;
	_vector vNextDir = XMVector3Normalize(vLen);

	_float fDis = XMVectorGetX(XMVector3Length(vLen));

	if (fDis < 3.f)
	{
		if (m_bLoop)
			++m_iIndex;
		else
			--m_iIndex;
	}


	_float3 vLastDir{};
	XMStoreFloat3(&vLastDir, vNextDir);

	m_pMoveIntent->SetFacingIntent(vLastDir, 60.f);
	m_pMoveIntent->SetMoveIntent(vLastDir, 30.f);
}

void CGriff::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	

}

void CGriff::FixedUpdate(E::_float fTimeDelta)
{
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
	
	//테스트용
	
}
void CGriff::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);

}

void CGriff::Set_Gravity(_bool bGravity)
{
	if (bGravity)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	else
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::DEL);
}

void CGriff::Set_Child()
{
	ANIMAL_DESC Child{};
	Child.TargetHandle = GetHandle();
	Child.sObjectTag = "GriffChild";
	Child.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	Child.ReSourceTag = "Model_Resource_Griff";
	_float3 vOffset = m_WayPoint.front();
	_float iCnt = 10.f;
	for (size_t i = 0; i < size_t(iCnt); ++i)
	{
		Child.vPos = _float3(vOffset.x + Randf(-iCnt, iCnt), 
			vOffset.y + Randf(-iCnt, iCnt), vOffset.z + Randf(-iCnt, iCnt));
			
		auto Griff = CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_GriffChild, "02_GriffChild", &Child);
		if (Griff)
			m_ChildHandles.push_back(Griff.value());
	}
	for (auto& iter : m_ChildHandles)
	{
		auto pChild = CGameInstance::Get().GetGameObjectByHandleT<CGriffChild>(iter);
		if (pChild)
			pChild->Set_Neighbor(m_ChildHandles);
	}
}

E::UPtr<CGriff> CGriff::Create()
{
	auto pInstance = E::ToUPtr(new CGriff{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CGriff");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CGriff::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CGriff{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CGriff");
		return nullptr;
	}

	return pInstance;
}
