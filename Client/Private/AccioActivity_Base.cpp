#include "pch.h"
#include "AccioActivity_Base.h"

#include "ComPxBoxCollider.h"
#include "ComPxRigidBody.h"
#include "ResPhysXBoxGeometry.h"
#include "ResPhysXMaterial.h"

NS_USING(Client)

CAccioActivity_Base::CAccioActivity_Base() = default;

CAccioActivity_Base::CAccioActivity_Base(const CAccioActivity_Base& prototype)
	: CAccioActivityPartBase{ prototype }
{
}

StringID CAccioActivity_Base::GetModelResourceTag() const
{
	return "Static_AccioActivity_Resource";
}

HRESULT CAccioActivity_Base::InitializeBasePhysics(const DESC& desc)
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

	for (size_t i = 0; i < desc.BoxColliders.size(); ++i)
	{
		const auto& box = desc.BoxColliders[i];
		CComPxBoxCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResMaterial = material;
		colliderDesc.pResBoxGeo = CResPhysXBoxGeometry::CreateAndLoad({
			.vHalfExtents = box.vHalfExtents });
		colliderDesc.vLocalOffset = box.vLocalOffset;
		colliderDesc.tFilter = desc.tPhysicsFilter;

		const _string componentTag = "ComPxBoxCollider_" + std::to_string(i);
		if (!colliderDesc.pResBoxGeo || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxBoxCollider",
			componentTag, &colliderDesc, &m_pComPxBoxColliders[i])))
		{
			return E_FAIL;
		}

		_float4 localRotation{};
		XMStoreFloat4(&localRotation, XMQuaternionRotationRollPitchYaw(
			box.vLocalRotation.x, box.vLocalRotation.y, box.vLocalRotation.z));
		if (!m_pComPxBoxColliders[i]->SetLocalRotation(localRotation))
			return E_FAIL;
	}

	return S_OK;
}

UPtr<CAccioActivity_Base> CAccioActivity_Base::Create()
{
	auto pInstance = ToUPtr(new CAccioActivity_Base{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CAccioActivity_Base::Clone(void* pArg)
{
	if (!pArg)
		return nullptr;

	auto pInstance = ToUPtr(new CAccioActivity_Base{ *this });
	if (FAILED(pInstance->Initialize(pArg)) ||
		FAILED(pInstance->InitializeBasePhysics(
			*static_cast<const DESC*>(pArg))))
	{
		return nullptr;
	}
	return pInstance;
}
