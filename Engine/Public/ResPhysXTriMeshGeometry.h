#pragma once
#include "ResPhysXGeometry.h"

NS_BEGIN(physx)
class PxTriangleMesh;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXTriMeshGeometry : public CResPhysXGeometry
{
public:
	DECLARE_DERIVED_TYPE(CResPhysXTriMeshGeometry, CResPhysXGeometry)

protected:
	explicit CResPhysXTriMeshGeometry(const _string& sPath);
	~CResPhysXTriMeshGeometry() override;

public:
	physx::PxTriangleMesh* GetTriMesh() const { return m_pTriMesh; }
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

protected:
	HRESULT CreateFromCookedData(uint8_t* pData, size_t iDataSize);

private:
	physx::PxTriangleMesh* m_pTriMesh{};

public:
	static SPtr<CResPhysXTriMeshGeometry> Create(const _string& sPath);
	static SPtr<CResPhysXTriMeshGeometry> CreateAndLoad(const _string& sPath);

private:
	void Free() override;
};

NS_END
