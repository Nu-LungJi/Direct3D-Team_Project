#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComStaticModelInstance;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

struct ACCIO_ACTIVITY_BOX_COLLIDER_DESC
{
	_float3 vHalfExtents{ 0.5f, 0.5f, 0.5f };
	_float3 vLocalOffset{};
	_float3 vLocalRotation{};
};

class CAccioActivityPartBase abstract : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CAccioActivityPartBase, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
	};

protected:
	CAccioActivityPartBase();
	CAccioActivityPartBase(const CAccioActivityPartBase& prototype);
	~CAccioActivityPartBase() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;

	bool IsOcclusionCullable() const override;
	bool GetOcclusionBounds(BoundingBox& outBounds) const override;

protected:
	virtual StringID GetModelResourceTag() const = 0;

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
};

NS_END
