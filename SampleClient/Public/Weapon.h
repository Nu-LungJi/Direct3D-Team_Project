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
NS_END

NS_BEGIN(Client)
class CWeapon final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CWeapon, CGameObject)

public:
	typedef struct tagWeapondesc : public CGameObject::GAMEOBJECT_DESC
	{

	}WEAPON_DESC;

private:
	CWeapon();
	~CWeapon() override;

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
	CComStaticModelInstance* m_pComModelInstance{};
	// nonAnim
	SPtr<CResPixelShader> m_pResPixelNonAnimShader{};
	SPtr<CResVertexShader> m_pResVertexNonAnimShader{};

	CComConstantBuffer* m_pComCBufferPerObject{};

public:
	static E::UPtr<CWeapon> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
