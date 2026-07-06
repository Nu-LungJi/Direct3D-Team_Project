#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
class CResPhysXTriMeshGeometry;
class CComPxRigidBody;
class CComPxTriMeshCollider;
class CResPhysXMaterial;
NS_END

NS_BEGIN(Client)
class CResTerrainVIBuffer;
class CTestPhysXTerrain final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTestPhysXTerrain, CGameObject)

public:
	typedef struct tagTerrainDesc : public CGameObject::GAMEOBJECT_DESC
	{
	}DESC;

private:
	CTestPhysXTerrain();
	~CTestPhysXTerrain() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	SPtr<CResTerrainVIBuffer> m_pResTerrainVIBuffer{};
	SPtr<CResTexture2D> m_pResTerrainTexture2D{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResSamplerState> m_pResSamplerState{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	std::vector<_float3> m_vecPoses{};
	std::vector<XMINT3> m_vecTriangles{};
	std::vector<VTX_COL> m_vecPreBuiltedDbgLineVertices{};

	SPtr<CResPhysXTriMeshGeometry> m_pResTriMesh{};
private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxTriMeshCollider* m_pComPxTriMeshCollider{};
public:
	static E::UPtr<CTestPhysXTerrain> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
