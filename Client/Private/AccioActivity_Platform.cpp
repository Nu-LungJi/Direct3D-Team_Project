#include "pch.h"
#include "AccioActivity_Platform.h"

#include "ComPxBoxCollider.h"
#include "ComPxConvexCollider.h"
#include "ComPxRigidBody.h"
#include "Engine_PhysxDefines.h"
#include "GameInstance.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"

NS_USING(Client)

CAccioActivity_Platform::CAccioActivity_Platform() = default;

CAccioActivity_Platform::CAccioActivity_Platform(
	const CAccioActivity_Platform& prototype)
	: CAccioActivityPartBase{ prototype }
{
}

StringID CAccioActivity_Platform::GetModelResourceTag() const
{
	return "Static_AccioActivity_Platform_Resource";
}

_bool CAccioActivity_Platform::GetNpcMoveAreaWorldOBB(
	BoundingOrientedBox& outArea) const
{
	if (!m_pComPxNpcMoveAreaTrigger)
		return false;

	const BoundingOrientedBox localArea{
		m_pComPxNpcMoveAreaTrigger->GetLocalPosition(),
		m_pComPxNpcMoveAreaTrigger->GetHalfExtents(),
		m_pComPxNpcMoveAreaTrigger->GetLocalRotation()
	};
	localArea.Transform(
		outArea,
		GetTransform().GetLoadedCombinedWorldMatrix());
	return true;
}

HRESULT CAccioActivity_Platform::InitializePlatformPhysics(const DESC& desc)
{
	{
		CComPxRigidBody::DESC rigidBodyDesc{};
		rigidBodyDesc.eType = CComPxRigidBody::TYPE::STATIC;
		rigidBodyDesc.vPosition = GetTransform().GetPosition();
		rigidBodyDesc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody",
			"ComPxRigidBody", &rigidBodyDesc, &m_pComPxRigidBody)))
		{
			return E_FAIL;
		}
	}

	auto material = CResPhysXMaterial::CreateAndLoad({});
	if (!material)
		return E_FAIL;

	{
		const auto& box = desc.BoxCollider;
		CComPxBoxCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResMaterial = material;
		colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = box.vHalfExtents });
		colliderDesc.vLocalOffset = box.vLocalOffset;
		colliderDesc.tFilter = desc.tPhysicsFilter;
		if (!colliderDesc.pResBoxGeo || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxBoxCollider",
			"ComPxBoxCollider", &colliderDesc, &m_pComPxBoxCollider)))
		{
			return E_FAIL;
		}

		_float4 localRotation{};
		XMStoreFloat4(&localRotation, XMQuaternionRotationRollPitchYaw(
			box.vLocalRotation.x, box.vLocalRotation.y, box.vLocalRotation.z));
		if (!m_pComPxBoxCollider->SetLocalRotation(localRotation))
			return E_FAIL;
	}

	{
		auto wedgeGeometry = CGameInstance::Get()
			.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
				PX_UNIT_WEDGE_CONVEX_PATH,
				[]()
				{
					return CResPhysXConvexGeometry::CreateAndLoad(
						PX_UNIT_WEDGE_CONVEX_PATH);
				});

		CComPxConvexCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResMaterial = material;
		colliderDesc.pResConvex = wedgeGeometry;
		colliderDesc.vScale = desc.WedgeCollider.vScale;
		colliderDesc.vLocalOffset = desc.WedgeCollider.vLocalOffset;
		colliderDesc.tFilter = desc.tPhysicsFilter;
		if (!wedgeGeometry || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxConvexCollider",
			"ComPxWedgeCollider", &colliderDesc, &m_pComPxWedgeCollider)))
		{
			return E_FAIL;
		}

		const auto& rotation = desc.WedgeCollider.vLocalRotation;
		_float4 localRotation{};
		XMStoreFloat4(&localRotation, XMQuaternionRotationRollPitchYaw(
			rotation.x, rotation.y, rotation.z));
		if (!m_pComPxWedgeCollider->SetLocalRotation(localRotation))
			return E_FAIL;
	}

	{
		// [LSY] NPC가 플랫폼 위에서 이동할 수 있는 범위를 저작하기 위한 Trigger다.
		// Object Manager의 NpcMoveAreaTrigger 컴포넌트 GUI에서 크기와 로컬 자세를 조절한다.
		const auto& moveArea = desc.NpcMoveAreaTrigger;
		CComPxBoxCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResMaterial = material;
		colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = moveArea.vHalfExtents });
		colliderDesc.vLocalOffset = moveArea.vLocalOffset;
		colliderDesc.tFilter = desc.tNpcMoveAreaTriggerFilter;
		colliderDesc.bIsTrigger = true;
		colliderDesc.iShapeSubIndex = NPC_MOVE_AREA_SHAPE_SUB_INDEX;
		if (!colliderDesc.pResBoxGeo || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxBoxCollider",
			"NpcMoveAreaTrigger", &colliderDesc, &m_pComPxNpcMoveAreaTrigger)))
		{
			return E_FAIL;
		}

		_float4 localRotation{};
		XMStoreFloat4(&localRotation, XMQuaternionRotationRollPitchYaw(
			moveArea.vLocalRotation.x,
			moveArea.vLocalRotation.y,
			moveArea.vLocalRotation.z));
		if (!m_pComPxNpcMoveAreaTrigger->SetLocalRotation(localRotation))
			return E_FAIL;
	}

	return S_OK;
}

UPtr<CAccioActivity_Platform> CAccioActivity_Platform::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_Platform{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_Platform::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CAccioActivity_Platform{ *this });
	if (FAILED(pInstance->Initialize(pArg)) ||
		FAILED(pInstance->InitializePlatformPhysics(
			*static_cast<const DESC*>(pArg))))
		return nullptr;
	return pInstance;
}
