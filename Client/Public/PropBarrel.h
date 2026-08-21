#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComPxConvexCollider;
class CComPxRigidBody;
class CComStaticModelInstance;
class CComConstantBuffer;
class CResPhysXConvexGeometry;
class CResPhysXMaterial;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CPropBarrel final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPropBarrel, CGameObject)

	enum class BARREL_STATE : uint8_t
	{
		CREATED,
		DESTROYED
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		StringID sResourceGroup{};
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 3.f, 3.f, 3.f };
		_float3 vConvexScale{ 3.f, 3.f, 3.f };
		_float3 vInitialImpulse{};
		_float3 vInitialAngularVelocityRadians{};
		_float fAngularDamping{ 0.05f };
		_float fMass{ 0.15f };
		_float fCollisionDestroySpeed{ 8.f };
		_float fCollisionDestroyGraceTime{ 0.3f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::PLAYER_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CPropBarrel();
	CPropBarrel(const CPropBarrel& prototype);
	~CPropBarrel() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	void UpdateGUI() override;
	void OnCollisionEnter(
		CGameObject* pObj,
		const PX_ON_COLLISION_DATA& info) override;

	// 완성된 통을 현재 자세 그대로 파편으로 교체한다.
	_bool DestroyBarrel();
	BARREL_STATE GetBarrelState() const { return m_eState; }


public:
	static UPtr<CPropBarrel> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	SPtr<CResPhysXConvexGeometry> m_pResConvexGeometry{};
	SPtr<CResPhysXMaterial> m_pResPhysXMaterial{};
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxConvexCollider* m_pComPxConvexCollider{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	StringID m_sResourceGroup{};
	_float3 m_vModelScale{ 1.f, 1.f, 1.f };
	_float3 m_vDebrisConvexScale{ 1.f, 1.f, 1.f };
	_float m_fCollisionDestroySpeed{ 8.f };
	_float m_fCollisionDestroyGraceTime{ 0.3f };
	_float m_fCollisionDestroyElapsed{};
	_float m_fCachedLinearSpeedSquared{};
	BARREL_STATE m_eState{ BARREL_STATE::CREATED };
	_bool m_bDestroyRequested{};
	_bool m_bDestroyOriginalNextFrame{};
};

NS_END
