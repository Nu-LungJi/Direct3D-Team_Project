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

class CPropBarrelDebris final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPropBarrelDebris, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		StringID sResourceGroup{};
		StringID sResourceTag{};
		std::string sConvexPath{};
		_float3 vInitialPosition{};
		_float4 vInitialQuaternion{ 0.f, 0.f, 0.f, 1.f };
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float3 vConvexScale{ 1.f, 1.f, 1.f };
		_float3 vInitialLinearVelocity{};
		_float3 vInitialAngularVelocityRadians{};
		_float fMass{ 0.15f };
		_float fDissolveDelay{ 3.f };
		_float fDissolveDuration{ 1.5f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::DEBRIS),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CPropBarrelDebris();
	CPropBarrelDebris(const CPropBarrelDebris& prototype);
	~CPropBarrelDebris() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;


public:
	static UPtr<CPropBarrelDebris> Create();
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
	_float3 m_vModelScale{ 1.f, 1.f, 1.f };
	_float m_fDissolveDelay{ 3.f };
	_float m_fDissolveDuration{ 1.5f };
	_float m_fLifeElapsed{};
	_float m_fDissolveIntensity{};
	_bool m_bDissolving{};
};

NS_END
