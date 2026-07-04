#pragma once

#include "Collider.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCollBox final : public CCollider
{
public:
	DECLARE_DERIVED_TYPE(CCollBox, CCollider)

private:
	explicit CCollBox();
	~CCollBox() override;

public:
	const BoundingBox& GetBoundingBox() const { return m_BoundingBoxWorld; }
	const BoundingBox& GetLocalBoundingBox() const { return m_BoundingBoxLocal; }
	void SetLocalBoundingBox(const _float3& vCenter, const _float3& vExtents) { m_BoundingBoxLocal = BoundingBox{vCenter, vExtents}; }

private:
	HRESULT Initialize(const _float3& vCenter, const _float3& vExtents);
	 void Transform(_fmatrix wordMatrix) override;
	 _bool Intersect(const CCollider& collider) const override;

private:
	BoundingBox m_BoundingBoxLocal{};
	BoundingBox m_BoundingBoxWorld{};

public:
	static UPtr<CCollBox> Create(const _float3& vCenter, const _float3& vExtents);
};

NS_END