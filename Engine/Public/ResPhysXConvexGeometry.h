#pragma once
#include "ResPhysXGeometry.h"

NS_BEGIN(physx)
class PxConvexMesh;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXConvexGeometry : public CResPhysXGeometry
{
public:
	DECLARE_DERIVED_TYPE(CResPhysXConvexGeometry, CResPhysXGeometry)

protected:
	explicit CResPhysXConvexGeometry(const _string& sPath);
	~CResPhysXConvexGeometry() override;

public:
	physx::PxConvexMesh* GetConvexMesh() const { return m_pConvexMesh; }
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

protected:
	HRESULT CreateFromCookedData(uint8_t* pData, size_t iDataSize);

private:
	physx::PxConvexMesh* m_pConvexMesh{};

public:
	static SPtr<CResPhysXConvexGeometry> Create(const _string& sPath);
	static SPtr<CResPhysXConvexGeometry> CreateAndLoad(const _string& sPath);

private:
	void Free() override;
};

NS_END
