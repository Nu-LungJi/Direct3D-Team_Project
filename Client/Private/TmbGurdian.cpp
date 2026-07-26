#include "pch.h"
#include "TmbGurdian.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "Weapon.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "DbgLineRender.h"
#include "TmbGurdianDead.h"
NS_USING(Client)

CTmbGurdian::CTmbGurdian()
{
}

CTmbGurdian::~CTmbGurdian()
{
}

void CTmbGurdian::UpdateGUI()
{
	__super::UpdateGUI();

}

HRESULT CTmbGurdian::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	return S_OK;
}

HRESULT CTmbGurdian::Initialize(void* pArg)
{
	auto MonDesc = static_cast<TMBGURDIAN_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 100;
	//피직스
	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.vPosition = MonDesc->vPos;
		Desc.tFilter = MonDesc->tFilter;
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
	Desc.LoadPath = MonDesc->BeHaviorTag;
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
		Desc.sGroupTag = MonDesc->LevelTag;
		Desc.sResTag = MonDesc->ReSourceTag;

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

	m_ParticleData.emplace(ATTMON::ATT_1, "SpawnSmokeJump.json");
	m_ParticleData.emplace(ATTMON::ATT_2, "SpawnSmoke1-1.json");

	m_pComTransform->SetRotation(XMVectorSet(MonDesc->vRot.x, MonDesc->vRot.y, MonDesc->vRot.z, 0.f), MonDesc->fAngle);
	m_pComTransform->SetScale(XMVectorSet(MonDesc->vScale.x, MonDesc->vScale.y, MonDesc->vScale.z, 0));


	// 죽음 파편들
	if(false)
	{
		for (uint32_t i = 0; i < 13; ++i)
		{
			CTmbGurdianDead::TMBGURDIAN_DEAD_DESC Desc{};
			Desc.sObjectTag = "TmbGurdianDead";
			Desc.DebrisResTag = "Static_Med_Debris_" + std::to_string(i);
			Desc.DebrisConvex = "./Resources/PhysX/Cooked/SM_Med_" + std::to_string(i) + ".pxconvex";
			Desc.vInitialPosition = { 5.f, 5.f, 5.f };
			auto debris = E::CGameInstance::Get().AddGameObjectToLayer(
				MonDesc->LevelTag,
				PROTO_GAMEOBJECT::Prototype_GameObject_TmbGurdianDead,
				"28_TmbGurdianDead",
				&Desc);
			if (!debris)
			{
				MSG_BOX("TmbGurdianDead AddLayer Failed");
				return E_FAIL;
			}
			m_vecDeadHandles.push_back(*debris);
		}
	}

	m_pModelAnimator->SetEvaluationMode(
		CComAnimator::EVALUATION_MODE::CPU_GPU);
	return S_OK;
}

void CTmbGurdian::PriorityUpdate(E::_float fTimeDelta)
{
	//if (m_iHp <= 0)
	//{
	//	for (auto& iter : m_DeadMeshes)
	//	{
	//		if(nullptr != iter)
	//			iter->PriorityUpdate(fTimeDelta);
	//	}
	//}else
		__super::PriorityUpdate(fTimeDelta);

}

void CTmbGurdian::FixedUpdate(E::_float fTimeDelta)
{
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}

void CTmbGurdian::Update(E::_float fTimeDelta)
{
	//if (m_iHp <= 0)
	//{
	//	for (auto& iter : m_DeadMeshes)
	//	{
	//		if (nullptr != iter)
	//			iter->Update(fTimeDelta);
	//	}
	//}
	//else
	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DEBRIS)))
	{
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DEBRIS), FLAGTYPE::DEL);
		volatile int x = 0;
	}
	__super::Update(fTimeDelta);

}

void CTmbGurdian::LateUpdate(E::_float fTimeDelta)
{
	//if (m_iHp <= 0)
	//{
	//	for (auto& iter : m_DeadMeshes)
	//	{
	//		if (nullptr != iter)
	//			iter->LateUpdate(fTimeDelta);
	//	}
	//}
	//else
		__super::LateUpdate(fTimeDelta);
}

E::UPtr<CTmbGurdian> CTmbGurdian::Create()
{
	auto pInstance = E::ToUPtr(new CTmbGurdian{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTmbGurdian");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTmbGurdian::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTmbGurdian{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTmbGurdian");
		return nullptr;
	}

	return pInstance;
}
