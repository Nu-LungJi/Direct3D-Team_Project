#pragma once

#include "Collider.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCollSphere final : public CCollider
{
public:
	DECLARE_DERIVED_TYPE(CCollSphere, CCollider)

private:
	explicit CCollSphere();
	~CCollSphere() override;

public:
	const BoundingSphere& GetBoundingSphere() const { return m_BoundingSphereWorld; }

public:
	static constexpr uint32_t SliceCnt = 16;
	static constexpr uint32_t CornerCnt = SliceCnt * 3 * 2;
	void GetCorners(_float3* Corners) const;

public:
	 void Transform(_fmatrix wordMatrix) override;
	 _bool Intersect(const CCollider& collider) const override;

private:
	HRESULT Initialize(const _float3 vCenter, _float fRadius);

private:
	BoundingSphere m_BoundingSphereLocal{};
	BoundingSphere m_BoundingSphereWorld{};

public:
	static UPtr<CCollSphere> Create(const _float3 vCenter, _float fRadius);
};

NS_END
