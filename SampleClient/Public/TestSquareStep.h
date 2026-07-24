#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComPxBoxCollider;
class CComPxRigidBody;
class CComStaticModelInstance;
class CResPhysXBoxGeometry;
class CResPhysXMaterial;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CTestSquareStep final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestSquareStep, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_bool bEnablePhysics{ true };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::MOVING_PLATFORM),
			.iSimulationMask =
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::PLAYER_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY) |
				ETOUI(COLLISION_LAYER::NPC_BODY) |
				ETOUI(COLLISION_LAYER::DEBRIS),
			.iQueryMask = PX_ALL_LAYERS
		};
	};

private:
	CTestSquareStep();
	CTestSquareStep(const CTestSquareStep& prototype);
	~CTestSquareStep() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;

	void SetHeightTarget(_float fTargetY, _float fMoveSpeed);
	const _float3& GetBasePosition() const { return m_vBasePosition; }

public:
	static UPtr<CTestSquareStep> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxBoxCollider* m_pComPxBoxCollider{};
	SPtr<CResPhysXBoxGeometry> m_pResBoxGeometry{};
	SPtr<CResPhysXMaterial> m_pResPhysXMaterial{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	_float3 m_vBasePosition{};
	_float m_fTargetY{};
	_float m_fMoveSpeed{};
};

NS_END
