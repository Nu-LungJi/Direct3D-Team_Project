#include "pch.h"
#include "CollOrientedBox.h"

#include "CollBox.h"
#include "CollSphere.h"
#include "CollFrustum.h"

NS_USING(Engine)
CCollOrientedBox::CCollOrientedBox()
    : CCollider{CollType::OrientedBox}
{
}
CCollOrientedBox::~CCollOrientedBox()
{
}

void CCollOrientedBox::Transform(_fmatrix wordMatrix)
{
	m_BoundingOrientedBoxLocal.Transform(m_BoundingOrientedBoxWorld, wordMatrix);
}

_bool CCollOrientedBox::Intersect(const CCollider& collider) const
{
	switch (collider.GetCollType())
	{
	case CollType::Box:
		return m_BoundingOrientedBoxWorld.Intersects(static_cast<const CCollBox&>(collider).GetBoundingBox());
	case CollType::OrientedBox:
		return m_BoundingOrientedBoxWorld.Intersects(static_cast<const CCollOrientedBox&>(collider).GetBoundingOrientedBox());
	case CollType::Sphere:
		return m_BoundingOrientedBoxWorld.Intersects(static_cast<const CCollSphere&>(collider).GetBoundingSphere());
	case CollType::Frustum:
		return m_BoundingOrientedBoxWorld.Intersects(static_cast<const CCollFrustum&>(collider).GetBoundingFrustum());
	}
	return false;
}

HRESULT CCollOrientedBox::Initialize(const _float3& vCenter, const _float3& vExtents, const _float4& vOrientation)
{
	m_BoundingOrientedBoxWorld = m_BoundingOrientedBoxLocal = BoundingOrientedBox{ vCenter, vExtents, vOrientation };
    return S_OK;
}

UPtr<CCollOrientedBox> CCollOrientedBox::Create(const _float3& vCenter, const _float3& vExtents, const _float4& vOrientation)
{
	auto pInstance = ToUPtr(new CCollOrientedBox{});
	if (FAILED(pInstance->Initialize(vCenter, vExtents, vOrientation)))
	{
		return nullptr;
	}
	return pInstance;
}
