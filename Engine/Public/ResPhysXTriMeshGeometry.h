#pragma once
#include "ResPhysXGeometry.h"

NS_BEGIN(physx)
class PxTriangleMesh;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXTriMeshGeometry final : public CResPhysXGeometry
{
public:
	struct DESC
	{
		std::vector<_float3>* pVecVertices{};
		std::vector<XMINT3>* pVecTriangles{};
	};
public:
	DECLARE_DERIVED_TYPE(CResPhysXTriMeshGeometry, CResPhysXGeometry)

private:
	explicit CResPhysXTriMeshGeometry(const _string& sPath);
	~CResPhysXTriMeshGeometry() override;

public:
	physx::PxTriangleMesh* GetTriMesh() const { return m_pTriMesh; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	physx::PxTriangleMesh* m_pTriMesh{};
public:
	static SPtr<CResPhysXTriMeshGeometry> Create();
};

NS_END
