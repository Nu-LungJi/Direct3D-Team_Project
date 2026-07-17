#pragma once
#include "Component.h"

namespace physx
{
	class PxShape;
	class PxMaterial;
	class PxGeometry;
}

NS_BEGIN(Engine)
class CComPxRigidBody;
class CResPhysXMaterial;
class ENGINE_DLL CComPxCollider : public CComponent
{
public:
	struct DESC : CComponent::DESC
	{
		CComPxRigidBody* pComPxRigidBody{};
		SPtr<CResPhysXMaterial> pResMaterial{};
		PX_FILTER_DESC tFilter{};

		//float    fStaticFriction = 0.5f;
		//float    fDynamicFriction = 0.5f;
		//float    fRestitution = 0.1f;
		bool     bIsTrigger = false;
		XMFLOAT3 vLocalOffset = { 0.f, 0.f, 0.f };
	};
public:
	DECLARE_DERIVED_TYPE(CComPxCollider, CComponent)

public:
	void UpdateGUI() override;

protected:
	explicit CComPxCollider();
	~CComPxCollider() override;

protected:
	HRESULT Initialize(void* pArg) override;
	_bool RegisterShape(PX_SHAPE_TYPE eType, uint32_t iSubIndex = std::numeric_limits<uint32_t>::max());

protected:
	CComPxRigidBody* m_pComRigidBody{};

protected:
	physx::PxShape* m_pShape = nullptr;
	SPtr<CResPhysXMaterial> m_pResMaterial{};
	PX_FILTER_DESC m_tFilter{};
	//physx::PxMaterial* m_pMaterial = nullptr;

protected:
	void Free() override;
};

NS_END
