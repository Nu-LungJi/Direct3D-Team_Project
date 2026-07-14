#pragma once

#include "Collider.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCollFrustum final : public CCollider
{
public:
	DECLARE_DERIVED_TYPE(CCollFrustum, CCollider)

public:
	const BoundingFrustum& GetBoundingFrustum() const { return m_BoundingFrustumWorld; }
	const BoundingFrustum& GetLocalBoundingFrustum() const { return m_BoundingFrustumLocal; }
	void SetLocalFrustum(_fmatrix mat) { m_BoundingFrustumLocal = BoundingFrustum{ mat }; }

private:
	explicit CCollFrustum();
	~CCollFrustum() override;

public:
	 void Transform(_fmatrix wordMatrix) override;
	 _bool Intersect(const CCollider& collider) const override;
	 _bool Intersect(const _float3& vOrigin, const _float3& vDir, _float& fDist) const override;

	 void	GetCorners(_float3* Corners) const;

	 void	Set_FrustumVolume(XMMATRIX _InvViewProjMatrix) { InvViewProjMatrix = _InvViewProjMatrix; };

private:
	HRESULT Initialize(_fmatrix mat);

private:
	BoundingFrustum m_BoundingFrustumLocal{};
	BoundingFrustum m_BoundingFrustumWorld{};

	XMMATRIX		InvViewProjMatrix;

public:
	static UPtr<CCollFrustum> Create(_fmatrix mat);
};

NS_END
