#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComPxBoxCollider;
class CComPxD6Joint;
class CComPxRigidBody;
NS_END

NS_BEGIN(Client)

class CPhysicsDoor final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPhysicsDoor, CGameObject)

	enum class HINGE_SIDE : uint8_t
	{
		LEFT,
		RIGHT
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vHalfExtents{ 1.5f, 2.5f, 0.15f };
		_float fMass{ 25.f };
		_float fAngularDamping{ 3.f };
		_float fLowerLimitDegrees{ -110.f };
		_float fUpperLimitDegrees{ 110.f };
		HINGE_SIDE eHingeSide{ HINGE_SIDE::LEFT };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CPhysicsDoor();
	CPhysicsDoor(const CPhysicsDoor& prototype);
	~CPhysicsDoor() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	void UpdateGUI() override;

	_bool ApplyOpeningTorque(_float fTorque);
	_bool ResetDoor();
	_bool IsHingeReady() const { return m_pComPxD6Joint != nullptr; }
	_float GetOpeningAngleDegrees() const;

protected:
	void OnRegisteredToManager() override;

private:
	_bool CreateHingeJoint();
	void DrawDebugDoor();

public:
	static UPtr<CPhysicsDoor> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxBoxCollider* m_pComPxBoxCollider{};
	CComPxD6Joint* m_pComPxD6Joint{};

	_float3 m_vHalfExtents{ 1.5f, 2.5f, 0.15f };
	_float3 m_vInitialPosition{};
	_float4 m_vInitialRotation{ 0.f, 0.f, 0.f, 1.f };
	_float m_fLowerLimitDegrees{ -110.f };
	_float m_fUpperLimitDegrees{ 110.f };
	_float m_fTestTorque{ 800.f };
	HINGE_SIDE m_eHingeSide{ HINGE_SIDE::LEFT };
	_bool m_bDebugDraw{ true };
};

NS_END
