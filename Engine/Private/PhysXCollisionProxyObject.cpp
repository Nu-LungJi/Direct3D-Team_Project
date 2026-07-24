#include "pch.h"
#include "PhysXCollisionProxyObject.h"

#include "GameInstance.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCapsuleCollider.h"
#include "ComPxConvexCollider.h"
#include "ComPxTriMeshCollider.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXSphereGeometry.h"
#include "ResPhysXCapsuleGeometry.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXTriMeshGeometry.h"
#include "ResPhysXMaterial.h"

#include <filesystem>

NS_USING(Engine)

namespace
{
	constexpr _float MIN_GEOMETRY_SIZE = 0.005f;

	CComPxRigidBody::TYPE ToRigidBodyType(PX_COLLISION_PROXY_ACTOR_TYPE type)
	{
		switch (type)
		{
		case PX_COLLISION_PROXY_ACTOR_TYPE::DYNAMIC:
			return CComPxRigidBody::TYPE::DYNAMIC;
		case PX_COLLISION_PROXY_ACTOR_TYPE::KINEMATIC:
			return CComPxRigidBody::TYPE::KINEMATIC;
		case PX_COLLISION_PROXY_ACTOR_TYPE::STATIC:
		default:
			return CComPxRigidBody::TYPE::STATIC;
		}
	}
}

HRESULT CPhysXCollisionProxyObject::Initialize(void* pArg)
{
	auto* desc = static_cast<DESC*>(pArg);
	if (!desc || (!desc->pCollisionData && desc->sCollisionFileName.empty()))
		return E_FAIL;
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	PX_COLLISION_PROXY_FILE data{};
	if (FAILED(LoadCollisionData(*desc, data)))
		return E_FAIL;
	return BuildCollision(data);
}

HRESULT CPhysXCollisionProxyObject::LoadCollisionData(
	const DESC& desc, PX_COLLISION_PROXY_FILE& outData) const
{
	if (desc.pCollisionData)
	{
		outData = *desc.pCollisionData;
		return outData.iVersion == 3 ? S_OK : E_FAIL;
	}

	const std::string path = MakePxCollisionFilePath(desc.sCollisionFileName);
	if (!std::filesystem::exists(path))
	{
		DEBUG_LOG_STR(std::string("[PX][CollisionProxy] File not found: ") + path + "\n");
		return E_FAIL;
	}
	if (FAILED(CGameInstance::Get().JsonDeSerialize(path, outData, "CollisionProxies")))
		return E_FAIL;
	return outData.iVersion == 3 ? S_OK : E_FAIL;
}

HRESULT CPhysXCollisionProxyObject::BuildCollision(const PX_COLLISION_PROXY_FILE& data)
{
	auto material = CResPhysXMaterial::CreateAndLoad({});
	if (!material)
		return E_FAIL;

	size_t createdActorCount{};
	size_t createdShapeCount{};
	for (const auto& actor : data.actors)
	{
		if (!actor.bEnabled)
			continue;

		CComPxRigidBody::DESC rigidBodyDesc{};
		rigidBodyDesc.eType = ToRigidBodyType(actor.eType);
		rigidBodyDesc.fMass = std::max(actor.fMass, 0.001f);
		rigidBodyDesc.vPosition = actor.vPosition;
		rigidBodyDesc.vRotation = actor.vRotation;

		CComPxRigidBody* rigidBody{};
		const std::string rigidBodyTag = "ComPxRigidBody_" + std::to_string(actor.iID);
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody", rigidBodyTag,
			&rigidBodyDesc, &rigidBody)))
		{
			return E_FAIL;
		}
		m_pComPxRigidBodies.push_back(rigidBody);
		if (actor.eType != PX_COLLISION_PROXY_ACTOR_TYPE::STATIC &&
			!rigidBody->SetGravityEnabled(actor.bGravity))
		{
			return E_FAIL;
		}
		++createdActorCount;

		for (const auto& shape : actor.shapes)
		{
			if (!shape.bEnabled)
				continue;
			if (shape.eType == PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH &&
				(actor.eType == PX_COLLISION_PROXY_ACTOR_TYPE::DYNAMIC || shape.bTrigger))
			{
				DEBUG_LOG("[PX][CollisionProxy] Invalid Triangle Mesh configuration.\n");
				return E_FAIL;
			}

			const PX_FILTER_DESC filter{
				.iLayer = shape.iLayer,
				.iSimulationMask = shape.iSimulationMask,
				.iQueryMask = shape.iQueryMask
			};
			const std::string colliderTag = "ComPxCollider_" + std::to_string(shape.iID);
			CComPxCollider* collider{};

			auto setCommonDesc = [&](CComPxCollider::DESC& colliderDesc)
			{
				colliderDesc.pComPxRigidBody = rigidBody;
				colliderDesc.pResMaterial = material;
				colliderDesc.tFilter = filter;
				colliderDesc.bIsTrigger = shape.bTrigger;
				colliderDesc.vLocalOffset = shape.vLocalPosition;
			};

			switch (shape.eType)
			{
			case PX_COLLISION_PROXY_SHAPE_TYPE::BOX:
			{
				CComPxBoxCollider::DESC colliderDesc{};
				setCommonDesc(colliderDesc);
				colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({ .vHalfExtents = {
					std::max(std::abs(shape.vSize.x) * 0.5f, MIN_GEOMETRY_SIZE),
					std::max(std::abs(shape.vSize.y) * 0.5f, MIN_GEOMETRY_SIZE),
					std::max(std::abs(shape.vSize.z) * 0.5f, MIN_GEOMETRY_SIZE) } });
				CComPxBoxCollider* concrete{};
				if (!colliderDesc.pResBoxGeo || FAILED(AddComponentFromProto(
					"PHYSX", "Prototype_Component_ComPxBoxCollider", colliderTag, &colliderDesc, &concrete)))
					return E_FAIL;
				collider = concrete;
				break;
			}
			case PX_COLLISION_PROXY_SHAPE_TYPE::SPHERE:
			{
				CComPxSphereCollider::DESC colliderDesc{};
				setCommonDesc(colliderDesc);
				colliderDesc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({
					.fRadius = std::max(shape.fRadius, MIN_GEOMETRY_SIZE) });
				CComPxSphereCollider* concrete{};
				if (!colliderDesc.pResSphereGeo || FAILED(AddComponentFromProto(
					"PHYSX", "Prototype_Component_ComPxSphereCollider", colliderTag, &colliderDesc, &concrete)))
					return E_FAIL;
				collider = concrete;
				break;
			}
			case PX_COLLISION_PROXY_SHAPE_TYPE::CAPSULE:
			{
				CComPxCapsuleCollider::DESC colliderDesc{};
				setCommonDesc(colliderDesc);
				colliderDesc.pResCapsuleGeo = CResPhysXCapsuleGeometry::CreateAndLoad({
					.fRadius = std::max(shape.fRadius, MIN_GEOMETRY_SIZE),
					.fHalfHeight = std::max(shape.fHalfHeight, MIN_GEOMETRY_SIZE) });
				CComPxCapsuleCollider* concrete{};
				if (!colliderDesc.pResCapsuleGeo || FAILED(AddComponentFromProto(
					"PHYSX", "Prototype_Component_ComPxCapsuleCollider", colliderTag, &colliderDesc, &concrete)))
					return E_FAIL;
				collider = concrete;
				break;
			}
			case PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH:
			{
				if (shape.sCookedResourcePath.empty()) return E_FAIL;
				CComPxConvexCollider::DESC colliderDesc{};
				setCommonDesc(colliderDesc);
				colliderDesc.pResConvex = CGameInstance::Get()
					.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
						shape.sCookedResourcePath,
						[path = shape.sCookedResourcePath]()
						{
							return CResPhysXConvexGeometry::CreateAndLoad(path);
						});
				colliderDesc.vScale = shape.vScale;
				CComPxConvexCollider* concrete{};
				if (!colliderDesc.pResConvex || FAILED(AddComponentFromProto(
					"PHYSX", "Prototype_Component_ComPxConvexCollider", colliderTag, &colliderDesc, &concrete)))
					return E_FAIL;
				collider = concrete;
				break;
			}
			case PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH:
			{
				if (shape.sCookedResourcePath.empty()) return E_FAIL;
				CComPxTriMeshCollider::DESC colliderDesc{};
				setCommonDesc(colliderDesc);
				colliderDesc.pResTriMesh = CGameInstance::Get()
					.GetOrCreateResourceByPath<CResPhysXTriMeshGeometry>(
						shape.sCookedResourcePath,
						[path = shape.sCookedResourcePath]()
						{
							return CResPhysXTriMeshGeometry::CreateAndLoad(path);
						});
				colliderDesc.vScale = shape.vScale;
				CComPxTriMeshCollider* concrete{};
				if (!colliderDesc.pResTriMesh || FAILED(AddComponentFromProto(
					"PHYSX", "Prototype_Component_ComPxTriMeshCollider", colliderTag, &colliderDesc, &concrete)))
					return E_FAIL;
				collider = concrete;
				break;
			}
			}

			if (!collider || !collider->SetLocalRotation(shape.vLocalRotation) ||
				!collider->SetSimulationEnabled(shape.bSimulationEnabled) ||
				!collider->SetQueryEnabled(shape.bQueryEnabled))
			{
				return E_FAIL;
			}
			++createdShapeCount;
		}
	}

	DEBUG_LOG_STR(std::string("[PX][CollisionProxy] Created actors: ") +
		std::to_string(createdActorCount) + ", shapes: " + std::to_string(createdShapeCount) + "\n");
	return S_OK;
}

void CPhysXCollisionProxyObject::PriorityUpdate(_float fTimeDelta)
{
}

void CPhysXCollisionProxyObject::Update(_float fTimeDelta)
{
}

void CPhysXCollisionProxyObject::LateUpdate(_float fTimeDelta)
{
}

HRESULT CPhysXCollisionProxyObject::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	return S_OK;
}

UPtr<CPhysXCollisionProxyObject> CPhysXCollisionProxyObject::Create()
{
	auto instance = ToUPtr(new CPhysXCollisionProxyObject{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

UPtr<CPrototype> CPhysXCollisionProxyObject::Clone(void* pArg)
{
	auto instance = ToUPtr(new CPhysXCollisionProxyObject{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
