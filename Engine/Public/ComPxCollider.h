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

public:
	_bool SetTrigger(_bool bTrigger);
	_bool IsTrigger() const;
	_bool SetEnabled(_bool bEnabled);
	_bool IsEnabled() const;
	_bool SetSimulationEnabled(_bool bEnabled);
	_bool IsSimulationEnabled() const;
	_bool SetQueryEnabled(_bool bEnabled);
	_bool IsQueryEnabled() const;

	_bool SetLocalPosition(const _float3& vPosition);
	_float3 GetLocalPosition() const;
	_bool SetLocalRotation(const _float4& vQuaternion);
	_float4 GetLocalRotation() const;

	_bool SetFilter(const PX_FILTER_DESC& tFilter);
	const PX_FILTER_DESC& GetFilter() const { return m_tFilter; }

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
	_bool m_bIsTrigger{};
	_bool m_bSimulationEnabled{ true };
	//physx::PxMaterial* m_pMaterial = nullptr;

protected:
	void Free() override;
};

NS_END
