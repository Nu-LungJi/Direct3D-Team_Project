#include "pch.h"
#include "GurdianWeapon.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComBeHavior.h"
#include "ComModelInstance.h"
#include "ComPxConvexCollider.h"
#include "ComPxRigidBody.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"
#include "Trail_CPU.h"

NS_USING(Client)

CGurdianWeapon::CGurdianWeapon()
	: CMon_Weapon{}
{
}

CGurdianWeapon::~CGurdianWeapon()
{
}

void CGurdianWeapon::UpdateGUI()
{
	CMon_Weapon::UpdateGUI();

}

HRESULT CGurdianWeapon::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
		return E_FAIL;

	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (FAILED(m_pResPixelNonAnimShader->Load()))
		return E_FAIL;

	return S_OK;
}

HRESULT CGurdianWeapon::Initialize(void* pArg)
{
	if (!pArg)
		return E_INVALIDARG;

	const auto* pDesc = static_cast<DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(InitializeDebrisPhysics(*pDesc)))
		return E_FAIL;

	return S_OK;
}

void CGurdianWeapon::PriorityUpdate(E::_float fTimeDelta)
{
	if (m_bDead)
	{
		Dead_Parent(fTimeDelta);
		return;
	}

	__super::PriorityUpdate(fTimeDelta);

}

void CGurdianWeapon::Update(E::_float fTimeDelta)
{
	if (m_bDead)
		return;

	
	__super::Update(fTimeDelta);

	Weapon_Throw(fTimeDelta);

}

void CGurdianWeapon::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bDead)
	{
		if (auto iter = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle))
		{
			if (!m_bThrow)
			{
				if (auto pModel = iter->GetComponent<CComModelInstance>("ComCModelIntance"))
				{
					if (pModel->Get_CombinedBoneMatrices().size() > m_iBoneSocketIndex)
					{
						_matrix Par = XMLoadFloat4x4(&pModel->Get_CombinedBoneMatrices()[m_iBoneSocketIndex]);
						for (uint32_t i = 0; i < 3; ++i)
						{
							Par.r[i] = XMVector3Normalize(Par.r[i]);
						}
						XMStoreFloat4x4(&m_ParentMatrix, Par * XMLoadFloat4x4(pModel->GetGameObject()->GetTransform().GetWorldMatrix()));
					}
				}
			}
			if (auto pBT = iter->GetComponent<CComBeHavior>("Com_BT"))
			{
				if (!m_bThrow && pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW)))
				{
					m_bThrow = true;
					XMStoreFloat3(&m_vLook, pBT->GetGameObject()->GetTransform().GetState(STATE::LOOK));
					XMStoreFloat4x4(&m_ParentMatrix, (XMLoadFloat4x4(&m_ParentMatrix)));
				}
				else if (m_bThrow && !pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW)))
					m_bThrow = false;

				if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::DEAD)))
				{
					m_bDead = true;
					m_bDebrisPhysicsActivated = true;

				}
				if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::DISSOLVE)))
				{
					Weapon_CallBack();
					pBT->Set_Flag(ETOUI(CBTRoot::BTFLAG::DISSOLVE), FLAGTYPE::DEL);
				}
			}
		}


	}

	if (m_bDead && m_bDebrisPhysicsActivated)
	{
		UpdatePhysicData();
		GetTransform().Update();
	}
	else
	{
		GetTransform().SetParentWorldMatrix(m_ParentMatrix);
		GetTransform().Update();
	}
	


	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);

	/*----------- 광윤 추가 -----------*/
	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
	/*---------------------------------*/
}

void CGurdianWeapon::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
}

_bool CGurdianWeapon::ActivateDebrisPhysics()
{
	if (m_bDebrisPhysicsActivated)
		return true;

	if (!m_pComPxRigidBody || !m_pComPxConvexCollider)
		return false;

	// [LSY] DEBRIS 프레임의 최신 손 소켓 자세를 확정한 뒤 월드 자세를 캡처한다.
	if (!m_bThrow && !UpdateSocketParentMatrix())
		return false;

	GetTransform().SetParentWorldMatrix(m_ParentMatrix);
	GetTransform().Update();

	_vector vWorldScale{};
	_vector vWorldRotation{};
	_vector vWorldPosition{};
	if (!XMMatrixDecompose(
		&vWorldScale,
		&vWorldRotation,
		&vWorldPosition,
		GetTransform().GetLoadedCombinedWorldMatrix()))
	{
		return false;
	}

	_float3 vPosition{};
	_float4 vRotation{};
	_float3 vScale{};
	XMStoreFloat3(&vPosition, vWorldPosition);
	XMStoreFloat4(&vRotation, XMQuaternionNormalize(vWorldRotation));
	XMStoreFloat3(&vScale, vWorldScale);

	if (!m_pComPxRigidBody->SetPose(vPosition, vRotation) ||
		!m_pComPxRigidBody->SetLinearVelocity({}) ||
		!m_pComPxRigidBody->SetAngularVelocity({}) ||
		!m_pComPxConvexCollider->SetSimulationEnabled(true) ||
		!m_pComPxConvexCollider->SetQueryEnabled(true) ||
		!m_pComPxRigidBody->SetGravityEnabled(true) ||
		!m_pComPxRigidBody->WakeUp())
	{
		m_pComPxConvexCollider->SetSimulationEnabled(false);
		m_pComPxConvexCollider->SetQueryEnabled(false);
		m_pComPxRigidBody->SetGravityEnabled(false);
		m_pComPxRigidBody->PutToSleep();
		return false;
	}

	// [LSY] 이후에는 소켓 Parent가 아니라 PhysX 월드 자세만 적용한다.
	GetTransform().SetParentWorldMatrix(std::nullopt);
	GetTransform().SetPosition(vPosition);
	GetTransform().SetQuaternion(vRotation);
	GetTransform().SetScale(vScale);
	GetTransform().Update();

	m_bThrow = false;
	m_bDebrisPhysicsActivated = true;
	return true;
}
void CGurdianWeapon::Dead_Parent(_float fTimeDelta)
{
	m_fTick += fTimeDelta;
	_float t = m_fTick / 3.f;
	m_fDissolveintensity = 0 + (1 - 0) * t;
	if (t >= 1.f)
	{
		m_fDissolveintensity = 0.f;
		m_fTick = 0.f;
		SetPendingDestroy();
	
	}
}

HRESULT CGurdianWeapon::InitializeDebrisPhysics(const DESC& Desc)
{
	const char* pConvexPath = ResolveConvexPath(Desc.WeaponName);
	if (!pConvexPath || !m_pComModelInstance || !m_pComModelInstance->GetModel())
		return E_FAIL;

	{
		CComPxRigidBody::DESC RigidBodyDesc{};
		RigidBodyDesc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		RigidBodyDesc.fMass = std::max(Desc.fMass, 0.001f);
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody,
			"ComPxRigidBody",
			&RigidBodyDesc,
			&m_pComPxRigidBody)))
		{
			return E_FAIL;
		}
	}

	auto pConvexResource = CGameInstance::Get()
		.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
			pConvexPath,
			[pConvexPath]()
			{
				return CResPhysXConvexGeometry::CreateAndLoad(pConvexPath);
			});
	if (!pConvexResource)
		return E_FAIL;

	_vector vPreScale{};
	_vector vPreRotation{};
	_vector vPrePosition{};
	if (!XMMatrixDecompose(
		&vPreScale,
		&vPreRotation,
		&vPrePosition,
		XMLoadFloat4x4(&m_pComModelInstance->GetModel()->Get_PreTransformMatrix())))
	{
		return E_FAIL;
	}

	_float3 vModelPreScale{};
	XMStoreFloat3(&vModelPreScale, vPreScale);
	CComPxConvexCollider::DESC ColliderDesc{};
	ColliderDesc.pComPxRigidBody = m_pComPxRigidBody;
	ColliderDesc.pResConvex = std::move(pConvexResource);
	ColliderDesc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
	ColliderDesc.vScale = {
		std::max(std::abs(vModelPreScale.x * Desc.vScale.x * Desc.vOwnerScale.x), 0.001f),
		std::max(std::abs(vModelPreScale.y * Desc.vScale.y * Desc.vOwnerScale.y), 0.001f),
		std::max(std::abs(vModelPreScale.z * Desc.vScale.z * Desc.vOwnerScale.z), 0.001f)
	};
	ColliderDesc.tFilter = Desc.tFilter;
	if (!ColliderDesc.pResMaterial || FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::PHYSX,
		ES_EngineProtoPhysXComponent::Prototype_Component_ComPxConvexCollider,
		"ComPxConvexCollider",
		&ColliderDesc,
		&m_pComPxConvexCollider)))
	{
		return E_FAIL;
	}

	if (!m_pComPxConvexCollider->SetSimulationEnabled(false) ||
		!m_pComPxConvexCollider->SetQueryEnabled(false) ||
		!m_pComPxRigidBody->SetGravityEnabled(false) ||
		!m_pComPxRigidBody->SetLinearDamping(0.1f) ||
		!m_pComPxRigidBody->SetAngularDamping(0.5f) ||
		!m_pComPxRigidBody->SetMaxDepenetrationVelocity(3.f) ||
		!m_pComPxRigidBody->PutToSleep())
	{
		return E_FAIL;
	}

	m_bDebrisPhysicsActivated = false;
	return S_OK;
}

_bool CGurdianWeapon::UpdateSocketParentMatrix()
{
	auto* pParent = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle);
	if (!pParent || m_iBoneSocketIndex < 0)
		return false;

	auto* pModel = pParent->GetComponent<CComModelInstance>("ComCModelIntance");
	if (!pModel)
		return false;

	const auto& CombinedBoneMatrices = pModel->Get_CombinedBoneMatrices();
	const size_t iBoneIndex = static_cast<size_t>(m_iBoneSocketIndex);
	if (iBoneIndex >= CombinedBoneMatrices.size())
		return false;

	_matrix matSocket = XMLoadFloat4x4(&CombinedBoneMatrices[iBoneIndex]);
	for (uint32_t i = 0; i < 3; ++i)
	{
		const _float fLengthSq = XMVectorGetX(XMVector3LengthSq(matSocket.r[i]));
		if (fLengthSq <= std::numeric_limits<_float>::epsilon())
			return false;
		matSocket.r[i] = XMVector3Normalize(matSocket.r[i]);
	}

	XMStoreFloat4x4(
		&m_ParentMatrix,
		matSocket * pParent->GetTransform().GetLoadedWorldMatrix());
	return true;
}

const char* CGurdianWeapon::ResolveConvexPath(std::string_view sWeaponName)
{
	if (sWeaponName == "Model_Resource_Axe")
		return "./Resources/PhysX/Cooked/SM_Tomb_Axe.pxconvex";
	if (sWeaponName == "Model_Resource_Mace")
		return "./Resources/PhysX/Cooked/SM_Tomb_Mace.pxconvex";
	if (sWeaponName == "Model_Resource_Sword")
		return "./Resources/PhysX/Cooked/SM_Tomb_Sword.pxconvex";
	return nullptr;
}

/*---------------------------------*/
void CGurdianWeapon::Weapon_Throw(_float fTimeDelta)
{
	if (!m_bThrow || m_bDebrisPhysicsActivated)
		return;
	m_fAngle = 30.f;
	_vector vTargetLook = XMVector3Normalize(XMLoadFloat3(&m_vLook));

	_matrix Rot = XMMatrixRotationQuaternion(XMQuaternionRotationAxis(XMVectorSet(0, 0, 1, 0), XMConvertToRadians(m_fAngle)));
	_matrix matRot = Rot * XMLoadFloat4x4(&m_ParentMatrix);
	matRot.r[3] += vTargetLook * 15.f * fTimeDelta;
	XMStoreFloat4x4(&m_ParentMatrix, matRot);

}

E::UPtr<CGurdianWeapon> CGurdianWeapon::Create()
{
	auto pInstance = E::ToUPtr(new CGurdianWeapon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CGurdianWeapon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CGurdianWeapon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CGurdianWeapon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CGurdianWeapon");
		return nullptr;
	}

	return pInstance;
}
