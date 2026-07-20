#include "pch.h"
#include "MapCollisionProxyObject.h"

#include "GameInstance.h"
#include "ComPxRigidBody.h"
#include "ComPxBoxCollider.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"

#include <filesystem>

NS_USING(Client)

namespace
{
	constexpr E::_float MIN_HALF_EXTENT = 0.005f;
}

HRESULT CMapCollisionProxyObject::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc || pDesc->sCollisionFileName.empty())
		return E_FAIL;

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	const std::string collisionFilePath = E::MakePxCollisionFilePath(pDesc->sCollisionFileName);
	if (!std::filesystem::exists(collisionFilePath))
	{
		DEBUG_LOG_STR(std::string("[PX][MapCollisionProxy] File not found: ") + collisionFilePath + "\n");
		return E_FAIL;
	}

	E::PX_COLLISION_PROXY_FILE data{};
	if (FAILED(CGameInstance::Get().JsonDeSerialize(
		collisionFilePath, data, "CollisionProxies")))
		return E_FAIL;

	if (data.iVersion != 1)
	{
		DEBUG_LOG("[PX][MapCollisionProxy] Unsupported file version.\n");
		return E_FAIL;
	}

	CComPxRigidBody::DESC rigidBodyDesc{};
	rigidBodyDesc.eType = CComPxRigidBody::TYPE::STATIC;
	if (FAILED(AddComponentFromProto(
		"PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody",
		&rigidBodyDesc, &m_pComPxRigidBody)))
		return E_FAIL;

	auto material = CGameInstance::Get().GetResourceFirst<CResPhysXMaterial>(
		"SAMPLE_CLIENT_PX", "TMP_MATERIAL");
	if (!material)
		material = CResPhysXMaterial::CreateAndLoad({});
	if (!material)
		return E_FAIL;

	size_t iCreatedCount{};
	for (const auto& box : data.boxes)
	{
		if (!box.bEnabled)
			continue;

		const E::_float3 halfExtents{
			std::max(std::abs(box.vSize.x) * 0.5f, MIN_HALF_EXTENT),
			std::max(std::abs(box.vSize.y) * 0.5f, MIN_HALF_EXTENT),
			std::max(std::abs(box.vSize.z) * 0.5f, MIN_HALF_EXTENT)
		};

		CComPxBoxCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({ .vHalfExtents = halfExtents });
		colliderDesc.pResMaterial = material;
		colliderDesc.vLocalOffset = box.vPosition;
		colliderDesc.tFilter = PX_FILTER_DESC{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};
		if (!colliderDesc.pResBoxGeo)
			return E_FAIL;

		CComPxBoxCollider* pCollider{};
		const std::string componentTag = "ComPxBoxCollider_" + std::to_string(box.iID);
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxBoxCollider", componentTag,
			&colliderDesc, &pCollider)))
			return E_FAIL;

		E::_float4 rotation{};
		XMStoreFloat4(&rotation, XMQuaternionNormalize(XMLoadFloat4(&box.vRotation)));
		if (!pCollider->SetLocalRotation(rotation))
			return E_FAIL;

		++iCreatedCount;
	}

	DEBUG_LOG_STR(std::string("[PX][MapCollisionProxy] Created boxes: ") +
		std::to_string(iCreatedCount) + "\n");
	return S_OK;
}

void CMapCollisionProxyObject::PriorityUpdate(E::_float fTimeDelta)
{
}

void CMapCollisionProxyObject::Update(E::_float fTimeDelta)
{
}

void CMapCollisionProxyObject::LateUpdate(E::_float fTimeDelta)
{
}

HRESULT CMapCollisionProxyObject::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

E::UPtr<CMapCollisionProxyObject> CMapCollisionProxyObject::Create()
{
	auto pInstance = E::ToUPtr(new CMapCollisionProxyObject{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

E::UPtr<E::CPrototype> CMapCollisionProxyObject::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CMapCollisionProxyObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
