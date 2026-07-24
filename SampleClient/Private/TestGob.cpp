#include "pch.h"
#include "TestGob.h"
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
NS_USING(Client)

CTestGob::CTestGob()
{
}

CTestGob::~CTestGob()
{
}

void CTestGob::UpdateGUI()
{
	__super::UpdateGUI();

}

HRESULT CTestGob::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	return S_OK;
}

HRESULT CTestGob::Initialize(void* pArg)
{
	auto MonDesc = static_cast<MONSTER_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 100;

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
	CWeapon::WEAPON_DESC WeaponDesc{};
	WeaponDesc.sObjectTag = "Weapon";
	WeaponDesc.ParentHandle = GetHandle();
	WeaponDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_RightHandSocket");
	WeaponDesc.WeaponName = "Static_Mace_Model_Resource";
	WeaponDesc.LevelTag = MonDesc->LevelTag;
	auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer(MonDesc->LevelTag, "Prototype_GameObject_Weapon", "03_Weapon", &WeaponDesc);
	if (!Weapon.has_value())
	{
		MSG_BOX("Create Failed Weapon");
		return E_FAIL;
	}
	m_Partes[ETOUI(PARTES::WEAPON)] = Weapon.value();

	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	test[ETOUI(ATTMON::ATT_1)] = CGameInstance::Get().Parse_Command("SpawnSmokeJump.json");
	test[ETOUI(ATTMON::ATT_2)] = CGameInstance::Get().Parse_Command("SpawnSmoke1-1.json");

	m_ParticleData.emplace(ATTMON::ATT_1, "SpawnSmokeJump.json");
	m_ParticleData.emplace(ATTMON::ATT_2, "SpawnSmoke1-1.json");
	return S_OK;
}

void CTestGob::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

}

void CTestGob::FixedUpdate(E::_float fTimeDelta)
{
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}

void CTestGob::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);

}

void CTestGob::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
}

E::UPtr<CTestGob> CTestGob::Create()
{
	auto pInstance = E::ToUPtr(new CTestGob{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTestGob");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTestGob::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTestGob{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTestGob");
		return nullptr;
	}

	return pInstance;
}
