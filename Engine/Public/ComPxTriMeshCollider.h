#pragma once
#include "ComPxCollider.h"

namespace physx
{
	class PxTriangleMesh;
}

NS_BEGIN(Engine)
class CResPhysXTriMeshGeometry;
class ENGINE_DLL CComPxTriMeshCollider : public CComPxCollider
{
public:
	struct DESC : CComPxCollider::DESC
	{
		SPtr<CResPhysXTriMeshGeometry> pResTriMesh{};
	};
public:
	DECLARE_DERIVED_TYPE(CComPxTriMeshCollider, CComPxCollider)

public:
	void UpdateGUI() override;

private:
	explicit CComPxTriMeshCollider();
	~CComPxTriMeshCollider() override;

public:
	HRESULT Initialize(void* pArg) override;

public:
	static UPtr<CComPxTriMeshCollider> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	SPtr<CResPhysXTriMeshGeometry> m_pResTriMesh{};
private:
	void Free() override;
};

NS_END
