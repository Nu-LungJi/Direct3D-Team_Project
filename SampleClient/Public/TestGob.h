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
class CComModelInstance;
class CComAnimator;
class CComBeHavior;
class CComCollider;
NS_END

NS_BEGIN(Client)
class CTestGob final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestGob, CGameObject)

public:
	typedef struct tagMonsterDesc : public CGameObject::GAMEOBJECT_DESC
	{
		_string SocketName;

	}MONSTER_DESC;

private:
	CTestGob();
	~CTestGob() override;

public:
	void UpdateGUI() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	CComModelInstance* m_pComModelInstance{};
	CComAnimator* m_pModelAnimator{};
	CComBeHavior* m_pBeHavior;
	CComCollider* m_pComCollider{};
	UPtr<CGameObject> m_Partes[ETOUI(PARTES::END)]{};

	// nonAnim
	SPtr<CResPixelShader> m_pResPixelNonAnimShader{};
	SPtr<CResVertexShader> m_pResVertexNonAnimShader{};
	// Anim
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexShader{};



	SPtr<CResSamplerState> m_pResSamplerState{};
	CComConstantBuffer* m_pComCBufferPerObject{};

	_float4 m_fAlbedoColor = { 1.f, 1.f, 1.f, 1.f };
	_float	m_fNormalIntensity = 1.f;
	_float	m_fRoughnessIntensity = 1.f;
	_float	m_fMetallicIntensity = 1.f;
	_float	m_fAmbientIntensity = 1.f;
	_float	m_fSpecularIntensity = 1.f;
	_float3 m_fEmissiveColor = { 1.f, 1.f, 1.f };
	_float	m_fEmissiveIntensity = 0.f;

	int32_t						m_iHp{};
	_bool						m_bDead{ false };
	_string						m_SocketName{};
public:
	static E::UPtr<CTestGob> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
