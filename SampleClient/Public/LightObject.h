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
NS_END

NS_BEGIN(Client)
class CLightObject final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CLightObject, CGameObject)

public:
	typedef struct tagTerrainDesc : public CGameObject::GAMEOBJECT_DESC
	{
	}DESC;

private:
	CLightObject();
	~CLightObject() override;

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

	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResSamplerState> m_pResSamplerState{};
	CComConstantBuffer* m_pComCBufferPerObject{};

	XMFLOAT3	m_fAlbedoValue{};
	_float		m_fRoughnessValue{};
	_float		m_fMetallicValue{};

public:
	static E::UPtr<CLightObject> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END