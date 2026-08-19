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
		StringID sResorceTag{};
		std::string sConvexPath{};
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float3 vConvexScale{ 1.f, 1.f, 1.f };
		_float fMass{ 0.15f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::DEBRIS),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::DEBRIS),
			//|
			//	ETOUI(COLLISION_LAYER::ENEMY_BODY) |
			//	ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
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
};

NS_END
