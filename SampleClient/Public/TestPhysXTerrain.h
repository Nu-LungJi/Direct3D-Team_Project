#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
class CResPhysXRTTriMeshGeometry;
class CComPxRigidBody;
class CComPxTriMeshCollider;
class CComPxConvexCollider;
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
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
			.iSimulationMask = PX_ALL_LAYERS,
			.iQueryMask = PX_ALL_LAYERS,
			.iNotifyFlags = PX_NOTIFY_ALL
		};
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
	void OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;

private:
	SPtr<CResTerrainVIBuffer> m_pResTerrainVIBuffer{};
	SPtr<CResTexture2D> m_pResTerrainTexture2D{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResSamplerState> m_pResSamplerState{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	std::vector<VTX_COL> m_vecPreBuiltedDbgLineVertices{};

	SPtr<CResPhysXRTTriMeshGeometry> m_pResTriMesh{};
	SPtr<CResPhysXRTConvexGeometry> m_pResConvex{};
private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxTriMeshCollider* m_pComPxTriMeshCollider{};
	CComPxConvexCollider* m_pComPxConvexCollider{};
public:
	static E::UPtr<CTestPhysXTerrain> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
