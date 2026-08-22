#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComPxRigidBody;
class CComPxSphereCollider;
class CComStaticModelInstance;
class CResPhysXMaterial;
class CResPhysXSphereGeometry;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CAccioBall final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CAccioBall, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		StringID sResourceGroup{};
		StringID sModelResourceTag{};
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float fSphereRadius{ 0.5f };
		_float fMass{ 1.f };
		_float fLinearDamping{ 0.4f };
		_float fAngularDamping{ 0.8f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::PLAYER_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY),
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CAccioBall();
	CAccioBall(const CAccioBall& prototype);
	~CAccioBall() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;

	_bool ApplyImpulse(const _float3& vImpulse);
	_bool ApplyTorque(const _float3& vTorque);
	_bool SetMotionTuning(
		_float fMass,
		_float fLinearDamping,
		_float fAngularDamping);
	_bool ResetToInitialPose();
	CComPxRigidBody* GetRigidBody() const { return m_pComPxRigidBody; }
	_float GetSphereRadius() const { return m_fSphereRadius; }

public:
	static UPtr<CAccioBall> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxSphereCollider* m_pComPxSphereCollider{};
	SPtr<CResPhysXMaterial> m_pResPhysXMaterial{};
	SPtr<CResPhysXSphereGeometry> m_pResSphereGeometry{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	_float m_fSphereRadius{ 0.5f };
	_float3 m_vInitialPosition{};
	_float4 m_vInitialRotation{ 0.f, 0.f, 0.f, 1.f };
};

NS_END
