#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
NS_END

NS_BEGIN(Client)
class CResTerrainVIBuffer;
class CLightTerrain final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CLightTerrain, CGameObject)

public:
	typedef struct tagTerrainDesc : public CGameObject::GAMEOBJECT_DESC
	{
	}DESC;

private:
	CLightTerrain();
	~CLightTerrain() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	HRESULT RenderDefault(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);
	HRESULT RenderShadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx);

private:
	SPtr<CResTerrainVIBuffer> m_pResTerrainVIBuffer{};
	SPtr<CResTexture2D> m_pResTerrainTexture2D{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	CComConstantBuffer* m_pComCBufferPerObject{};

public:
	static E::UPtr<CLightTerrain> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
