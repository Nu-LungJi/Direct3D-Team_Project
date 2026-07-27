#include "pch.h"
#include "MedDebris.h"

#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComPxConvexCollider.h"
#include "ComPxRigidBody.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"
#include "Resources.h"

NS_USING(Client)

namespace
{
	constexpr char TEST_DYNAMIC_CONVEX_PATH[] =
		"./Resources/PhysX/Cooked/SM_oil_barrel_0001.pxconvex";
}

CMedDebris::CMedDebris()
	: CGameObject{}
{
}

CMedDebris::CMedDebris(const CMedDebris& prototype)
	: CGameObject{ prototype }
	//, m_pResVertexShader{ prototype.m_pResVertexShader }
	//, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT CMedDebris::InitializePrototype(void* pArg)
{


	return S_OK;
}

HRESULT CMedDebris::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	const auto* desc = static_cast<DESC*>(pArg);
	GetTransform().SetPosition(desc->vInitialPosition);
	GetTransform().SetRotationEuler(desc->vInitialRotation);
	GetTransform().SetScale(desc->vInitialScale);

	{
		CComConstantBuffer::DESC bufferDesc{};
		bufferDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject",
			&bufferDesc, &m_pComCBufferPerObject)))
			return E_FAIL;
	}

	{
		
		// "LEVEL_CREATURE", "Static_Med_Debris"
		CComStaticModelInstance::DESC modelDesc{};
		modelDesc.sGroupTag = "LEVEL_CREATURE";
		//modelDesc.sResTag = "Static_Med_Debris_0";
		modelDesc.sResTag = desc->DebrisResTag;
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance", "ComModelInstance",
			&modelDesc, &m_pComModelInstance)))
			return E_FAIL;
	}

	{
		CComPxRigidBody::DESC rigidBodyDesc{};
		rigidBodyDesc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		rigidBodyDesc.fMass = std::max(desc->fMass, 0.001f);
		rigidBodyDesc.vPosition = desc->vInitialPosition;
		rigidBodyDesc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody",
			&rigidBodyDesc, &m_pComPxRigidBody)))
			return E_FAIL;
	}

	
	{
		auto path = desc->DebrisConvex;
		auto convexResource = CGameInstance::Get()
			.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
				path,
				[path]() { return CResPhysXConvexGeometry::CreateAndLoad(path); });
		if (!convexResource)
			return E_FAIL;

		CComPxConvexCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResConvex = std::move(convexResource);
		colliderDesc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		colliderDesc.vScale = {
			std::max(std::abs(desc->vConvexScale.x), 0.001f),
			std::max(std::abs(desc->vConvexScale.y), 0.001f),
			std::max(std::abs(desc->vConvexScale.z), 0.001f)
		};
		colliderDesc.tFilter = desc->tFilter;
		if (!colliderDesc.pResMaterial || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxConvexCollider", "ComPxConvexCollider",
			&colliderDesc, &m_pComPxConvexCollider)))
			return E_FAIL;
	}

	if (!m_pComPxConvexCollider->SetSimulationEnabled(false) ||
		!m_pComPxConvexCollider->SetQueryEnabled(true) ||
		!m_pComPxRigidBody->SetGravityEnabled(false) ||
		!m_pComPxRigidBody->SetMaxDepenetrationVelocity(1.f) ||
		!m_pComPxRigidBody->PutToSleep())
		return E_FAIL;

	m_bPhysicsEnabled = false;
	m_bPendingPhysicsActivation = false;
	return S_OK;
}

void CMedDebris::PriorityUpdate(E::_float fTimeDelta)
{
}

void CMedDebris::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bPendingPhysicsActivation || m_bPhysicsEnabled)
		return;

	m_bPendingPhysicsActivation = false;

	if (!m_pComPxRigidBody || !m_pComPxConvexCollider)
		return;

	const _bool bPoseResult = m_pComPxRigidBody->SetPose(
		GetTransform().GetPosition(),
		GetTransform().GetQuaternion());
	const _bool bLinearVelocityResult =
		m_pComPxRigidBody->SetLinearVelocity({});
	const _bool bAngularVelocityResult =
		m_pComPxRigidBody->SetAngularVelocity({});
	const _bool bSimulationResult =
		m_pComPxConvexCollider->SetSimulationEnabled(true);
	const _bool bGravityResult =
		m_pComPxRigidBody->SetGravityEnabled(true);
	const _bool bWakeResult = m_pComPxRigidBody->WakeUp();

	m_bPhysicsEnabled =
		bPoseResult &&
		bLinearVelocityResult &&
		bAngularVelocityResult &&
		bSimulationResult &&
		bGravityResult &&
		bWakeResult;
}

void CMedDebris::Update(E::_float fTimeDelta)
{
}

void CMedDebris::LateUpdate(E::_float fTimeDelta)
{
	UpdatePhysicData();
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
	}

	const auto& pModel = m_pComModelInstance->GetModel();
	if (!pModel->HasLocalBounds())
		return;

	MAPMESH_INSTANCE_DATA InstanceData{};
	XMStoreFloat4x4(
		&InstanceData.world,
		GetTransform().GetLoadedCombinedWorldMatrix());

	BoundingBox WorldBounds{};
	pModel->GetLocalBounds().Transform(
		WorldBounds,
		GetTransform().GetLoadedCombinedWorldMatrix());

	MAPMESH_OCCLUSION_DATA OcclusionData{};
	OcclusionData.worldCenter = WorldBounds.Center;
	OcclusionData.worldExtents = WorldBounds.Extents;

	CGameInstance::Get().PushMapObjectInstance(
		pModel,
		InstanceData,
		OcclusionData);
}

HRESULT CMedDebris::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	
	return S_OK;
}

void CMedDebris::RequestActivatePhysics()
{
	if (!m_bPhysicsEnabled)
		m_bPendingPhysicsActivation = true;
}

_bool CMedDebris::ApplyPushForce(const _float3& vDirection, _float fStrength)
{
	if (!m_bPhysicsEnabled || !m_pComPxRigidBody || fStrength <= 0.f)
		return false;

	const _vector vDirectionVector = XMLoadFloat3(&vDirection);
	if (XMVectorGetX(XMVector3LengthSq(vDirectionVector)) <= FLT_EPSILON)
		return false;

	_float3 vNormalizedDirection{};
	XMStoreFloat3(&vNormalizedDirection, XMVector3Normalize(vDirectionVector));
	return m_pComPxRigidBody->AddForce({
		vNormalizedDirection.x * fStrength,
		vNormalizedDirection.y * fStrength,
		vNormalizedDirection.z * fStrength });
}

E::UPtr<CMedDebris> CMedDebris::Create()
{
	auto instance = E::ToUPtr(new CMedDebris{});
	if (FAILED(instance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CMedDebris");
		return nullptr;
	}
	return instance;
}

E::UPtr<E::CPrototype> CMedDebris::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CMedDebris{ *this });
	if (FAILED(instance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CMedDebris");
		return nullptr;
	}
	return instance;
}
