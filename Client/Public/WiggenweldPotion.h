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

class CWiggenweldPotion final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CWiggenweldPotion, CGameObject)

	enum class STATE : uint8_t
	{
		HELD,
		DROPPED
	};

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		StringID sResourceGroup{};
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float3 vConvexScale{ 2.f, 2.f, 2.f };
		_float fMass{ 0.15f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::MOVING_PLATFORM),
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CWiggenweldPotion();
	CWiggenweldPotion(const CWiggenweldPotion& prototype);
	~CWiggenweldPotion() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;

	_bool SetHeldPose(_fmatrix worldMatrix);
	_bool Drop(const _float3& vLinearVelocity = {}, const _float3& vAngularVelocity = {});
	STATE GetState() const { return m_eState; }

public:
	static UPtr<CWiggenweldPotion> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	static constexpr _float POTION_DISSOLVE_DELAY = 10.f;
	static constexpr _float POTION_DISSOLVE_DURATION = 1.5f;

	SPtr<CResPhysXConvexGeometry> m_pResConvexGeometry{};
	SPtr<CResPhysXMaterial> m_pResPhysXMaterial{};
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxConvexCollider* m_pComPxConvexCollider{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	STATE m_eState{ STATE::HELD };
	_float3 m_vModelScale{ 1.f, 1.f, 1.f };
	_float m_fDroppedElapsed{};
	_float m_fDissolveIntensity{};
	_bool m_bDissolving{};
	_bool m_bRenderConfirmed{};
};

NS_END
