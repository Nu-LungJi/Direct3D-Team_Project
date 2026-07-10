#include "pch.h"
#include "CollSphere.h"

#include "CollOrientedBox.h"
#include "CollBox.h"
#include "CollFrustum.h"

NS_USING(Engine)
CCollSphere::CCollSphere()
    : CCollider{ CollType::Sphere }
{
}
CCollSphere::~CCollSphere()
{
}

void CCollSphere::GetCorners(_float3* Corners) const
{ 
    auto radius = m_BoundingSphereWorld.Radius;
    auto center = m_BoundingSphereWorld.Center;

    //const auto& color = m_DbgColor[pCollider];
    uint32_t idx = 0;
    // XY
    for (int i = 0; i < SliceCnt; ++i)
    {
        float t0 = XM_2PI * i / SliceCnt;
        float t1 = XM_2PI * (i + 1) / SliceCnt;

        _float3 p0 = {
            center.x + radius * cosf(t0),
            center.y + radius * sinf(t0),
            center.z
        };

        _float3 p1 = {
            center.x + radius * cosf(t1),
            center.y + radius * sinf(t1),
            center.z
        };

        Corners[idx++] = p0;
        Corners[idx++] = p1;
    }

    // XZ
    for (int i = 0; i < SliceCnt; ++i)
    {
        float t0 = XM_2PI * i / SliceCnt;
        float t1 = XM_2PI * (i + 1) / SliceCnt;

        _float3 p0 = {
            center.x + radius * cosf(t0),
            center.y,
            center.z + radius * sinf(t0)
        };

        _float3 p1 = {
            center.x + radius * cosf(t1),
            center.y ,
            center.z + radius * sinf(t1)
        };

        Corners[idx++] = p0;
        Corners[idx++] = p1;
    }

    // YZ
    for (int i = 0; i < SliceCnt; ++i)
    {
        float t0 = XM_2PI * i / SliceCnt;
        float t1 = XM_2PI * (i + 1) / SliceCnt;

        _float3 p0 = {
            center.x,
            center.y + radius * cosf(t0),
            center.z + radius * sinf(t0)
        };

        _float3 p1 = {
            center.x ,
            center.y + radius * cosf(t1),
            center.z + radius * sinf(t1)
        };

        Corners[idx++] = p0;
        Corners[idx++] = p1;
    }
}

void CCollSphere::Transform(_fmatrix wordMatrix)
{
    m_BoundingSphereLocal.Transform(m_BoundingSphereWorld, wordMatrix);
}

_bool CCollSphere::Intersect(const CCollider& collider) const
{
	switch (collider.GetCollType())
	{
	case CollType::Box:
		return m_BoundingSphereWorld.Intersects(static_cast<const CCollBox&>(collider).GetBoundingBox());
	case CollType::OrientedBox:
		return m_BoundingSphereWorld.Intersects(static_cast<const CCollOrientedBox&>(collider).GetBoundingOrientedBox());
	case CollType::Sphere:
		return m_BoundingSphereWorld.Intersects(static_cast<const CCollSphere&>(collider).GetBoundingSphere());
	case CollType::Frustum:
		return m_BoundingSphereWorld.Intersects(static_cast<const CCollFrustum&>(collider).GetBoundingFrustum());
	}
	return false;
}

_bool CCollSphere::Intersect(const _float3& vOrigin, const _float3& vDir, _float& fDist) const
{
	return m_BoundingSphereWorld.Intersects(XMLoadFloat3(&vOrigin), XMLoadFloat3(&vDir), fDist);
}
HRESULT CCollSphere::Initialize(const _float3 vCenter, _float fRadius)
{
    m_BoundingSphereWorld = m_BoundingSphereLocal = BoundingSphere{ vCenter, fRadius };
    return S_OK;
}

UPtr<CCollSphere> CCollSphere::Create(const _float3 vCenter, _float fRadius)
{
    auto pInstance = ToUPtr(new CCollSphere{});
    if (FAILED(pInstance->Initialize(vCenter, fRadius)))
    {
        return nullptr;
    }
    return pInstance;
}
