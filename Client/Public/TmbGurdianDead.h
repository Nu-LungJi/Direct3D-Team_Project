#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
class CResModel;
class CComStaticModelInstance;
class CComPxRigidBody;
class CComPxConvexCollider;
NS_END

NS_BEGIN(Client)
class CTmbGurdianDead final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTmbGurdianDead, CGameObject)

public:
	typedef struct tagTmbGuardianDeaddesc : public CGameObject::GAMEOBJECT_DESC
	{
		StringID sResourceGroup{};
		StringID DebrisResTag{};
		std::string DebrisConvex{};
		_float3 vInitialPosition{};
		_float4 vInitialQuaternion{ 0.f, 0.f, 0.f, 1.f };
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float3 vConvexScale{ 1.f, 1.f, 1.f };
		_float fMass{ 1.f };
		PX_FILTER_DESC tFilter{};
	}TMBGURDIAN_DEAD_DESC;

private:
	CTmbGurdianDead();
	~CTmbGurdianDead() override;

public:
	void UpdateGUI() override;
	void SetRenderEnabled(_bool bEnabled)
	{
		m_bRenderEnabled = bEnabled;
	}
	_bool ActivatePhysics();
	_bool ApplyBonePose(
		_fmatrix matSocketWorld,
		_fmatrix matInverseBind);
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

	/*----------- 광윤 추가 -----------*/
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	/*---------------------------------*/

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxConvexCollider* m_pComPxConvexCollider{};
	_bool m_bActivated{};
	_bool m_bSocketAttached{};
	_bool m_bRenderEnabled{};
	// nonAnim
	SPtr<CResPixelShader> m_pResPixelNonAnimShader{};
	SPtr<CResVertexShader> m_pResVertexNonAnimShader{};

	CComConstantBuffer* m_pComCBufferPerObject{};

public:
	static E::UPtr<CTmbGurdianDead> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
