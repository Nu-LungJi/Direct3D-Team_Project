#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComPxConvexCollider;
class CComPxRigidBody;
class CComStaticModelInstance;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class CMedDebris final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CMedDebris, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		StringID DebrisResTag{};
		std::string DebrisConvex{};
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float3 vConvexScale{ 1.f, 1.f, 1.f };
		_float fMass{ 1.f };
		PX_FILTER_DESC tFilter{};
	};

private:
	CMedDebris();
	CMedDebris(const CMedDebris& prototype);
	~CMedDebris() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void FixedUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	void RequestActivatePhysics();
	_bool ApplyPushForce(const _float3& vDirection, _float fStrength);

public:
	static E::UPtr<CMedDebris> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxConvexCollider* m_pComPxConvexCollider{};
	_bool m_bPhysicsEnabled{};
	_bool m_bPendingPhysicsActivation{};
};

NS_END
