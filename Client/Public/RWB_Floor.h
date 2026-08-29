#pragma once
#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComPxConvexCollider;
class CComPxDistanceJoint;
class CComPxFixedJoint;
class CComPxRigidBody;
class CComStaticModelInstance;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CRWB_Floor final : public CGameObject {
public:
	DECLARE_DERIVED_TYPE(CRWB_Floor, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
	};

private:
	CRWB_Floor();
	CRWB_Floor(const CRWB_Floor& prototype);
	~CRWB_Floor() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

	/*----------- 광윤 추가 -----------*/
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	bool GetShadowBounds(BoundingBox& OutBounds) const override;
	/*---------------------------------*/
public:
	static E::UPtr<CRWB_Floor> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxConvexCollider* m_pComPxConvexCollider{};
	CComPxFixedJoint* m_pComPxFixedJoint{};
	CComPxDistanceJoint* m_pComPxDistanceJoint{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResPixelShader> m_pResReflectionPixelShader{};

	_float m_fWheelRotation{};
	
};

NS_END
