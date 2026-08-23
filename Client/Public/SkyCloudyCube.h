#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CResCubeColBuffer;
class CResTextureCubeMap;
NS_END

NS_BEGIN(Client)

class CSkyCloudyCube final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CSkyCloudyCube, CGameObject)
public:
	typedef struct tagSkyDesc : CGameObject::GAMEOBJECT_DESC
	{
		std::string sTextureTag{ "TEX_SkyCloudyCube" };
	} SKY_DESC;

private:
	CSkyCloudyCube();
	~CSkyCloudyCube() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;

private:
	E::CComConstantBuffer* m_pComCBufferPerObject{};
	E::SPtr<E::CResCubeColBuffer> m_pCubeBuffer{};
	E::SPtr<E::CResTextureCubeMap> m_pCubeTexture{};

public:
	static E::UPtr<CSkyCloudyCube> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
