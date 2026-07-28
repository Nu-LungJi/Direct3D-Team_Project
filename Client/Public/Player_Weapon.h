

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
class CPlayer_Weapon final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Weapon, CGameObject)

public:
	typedef struct tagWeapondesc : public CGameObject::GAMEOBJECT_DESC
	{
		_string	WeaponName{},LevelTag{};
		CHandle ParentHandle{};
		int32_t iBoneIndex{ -1 };
	}WEAPON_DESC;

private:
	CPlayer_Weapon();
	~CPlayer_Weapon() override;

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
	void					Weapon_Throw(_float fTimeDelta);
private:
	CComStaticModelInstance* m_pComModelInstance{};
	// nonAnim
	SPtr<CResPixelShader> m_pResPixelNonAnimShader{};
	SPtr<CResVertexShader> m_pResVertexNonAnimShader{};

	CComConstantBuffer* m_pComCBufferPerObject{};

	_float4x4			m_ParentMatrix{};
	_float3				m_vLook{};
	CHandle				m_ParentHandle{};
	int32_t				m_iBoneSocketIndex{ -1 };
	_float				m_fAngle{ 0 };
	_bool				m_bThrow{ false };

private:
	_float3 m_vSpawnLocalOffset{ 0.f, 0.3f, 0.f };

public:
	_float4x4 GetSpawnWorldMatrix() const;
public:
	static E::UPtr<CPlayer_Weapon> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
