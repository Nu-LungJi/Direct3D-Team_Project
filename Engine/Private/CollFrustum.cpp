#include "pch.h"
#include "CollFrustum.h"

#include "CollOrientedBox.h"
#include "CollSphere.h"
#include "CollBox.h"

NS_USING(Engine)
CCollFrustum::CCollFrustum()
    :CCollider{CollType::Frustum}
{
}
CCollFrustum::~CCollFrustum()
{
}

void CCollFrustum::Transform(_fmatrix wordMatrix)
{
	m_BoundingFrustumLocal.Transform(m_BoundingFrustumWorld, wordMatrix);
}

_bool CCollFrustum::Intersect(const CCollider& collider) const
{
	switch (collider.GetCollType())
	{
	case CollType::Box:
		return m_BoundingFrustumWorld.Intersects(static_cast<const CCollBox&>(collider).GetBoundingBox());
	case CollType::OrientedBox:
		return m_BoundingFrustumWorld.Intersects(static_cast<const CCollOrientedBox&>(collider).GetBoundingOrientedBox());
	case CollType::Sphere:
		return m_BoundingFrustumWorld.Intersects(static_cast<const CCollSphere&>(collider).GetBoundingSphere());
	case CollType::Frustum:
		return m_BoundingFrustumWorld.Intersects(static_cast<const CCollFrustum&>(collider).GetBoundingFrustum());
	}
	return false;
}

_bool CCollFrustum::Intersect(const _float3& vOrigin, const _float3& vDir, _float& fDist) const
{
	return m_BoundingFrustumWorld.Intersects(XMLoadFloat3(&vOrigin), XMLoadFloat3(&vDir), fDist);
}

HRESULT CCollFrustum::Initialize(_fmatrix mat)
{
    m_BoundingFrustumWorld = m_BoundingFrustumLocal = BoundingFrustum{ mat };
    return S_OK;
}

UPtr<CCollFrustum> CCollFrustum::Create(_fmatrix mat)
{
	auto pInstance = ToUPtr(new CCollFrustum{});
	if (FAILED(pInstance->Initialize(mat)))
	{
		return nullptr;
	}
	return pInstance;
}
