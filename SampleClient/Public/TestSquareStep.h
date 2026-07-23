#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComPxBoxCollider;
class CComPxRigidBody;
class CComStaticModelInstance;
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
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_DYNAMIC),
			.iSimulationMask = PX_ALL_LAYERS,
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

public:
	static UPtr<CTestSquareStep> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxBoxCollider* m_pComPxBoxCollider{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
};

NS_END
