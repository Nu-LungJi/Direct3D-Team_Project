#include "pch.h"
#include "CollBox.h"

#include "CollOrientedBox.h"
#include "CollSphere.h"
#include "CollFrustum.h"

NS_USING(Engine)

CCollBox::CCollBox()
	: CCollider{ CollType::Box }
{
}
CCollBox::~CCollBox()
{
}

HRESULT CCollBox::Initialize(const _float3& vCenter, const _float3& vExtents)
{
	m_BoundingBoxWorld = m_BoundingBoxLocal = BoundingBox{ vCenter, vExtents };
	return S_OK;
}

void CCollBox::Transform(_fmatrix wordMatrix)
{
	m_BoundingBoxLocal.Transform(m_BoundingBoxWorld, wordMatrix);
}

_bool CCollBox::Intersect(const CCollider& collider) const
{
	switch (collider.GetCollType())
	{
	case CollType::Box:
		return m_BoundingBoxWorld.Intersects(static_cast<const CCollBox&>(collider).GetBoundingBox());
	case CollType::OrientedBox:
		return m_BoundingBoxWorld.Intersects(static_cast<const CCollOrientedBox&>(collider).GetBoundingOrientedBox());
	case CollType::Sphere:
		return m_BoundingBoxWorld.Intersects(static_cast<const CCollSphere&>(collider).GetBoundingSphere());
	case CollType::Frustum:
		return m_BoundingBoxWorld.Intersects(static_cast<const CCollFrustum&>(collider).GetBoundingFrustum());
	}

	return false;
}

UPtr<CCollBox> CCollBox::Create(const _float3& vCenter, const _float3& vExtents)
{
	auto pInstance = ToUPtr(new CCollBox{});
	if (FAILED(pInstance->Initialize(vCenter, vExtents)))
	{
		return nullptr;
	}
	return pInstance;
}
