#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class CCollider;
class ENGINE_DLL CComCollider : public CComponent
{
public:
	typedef struct tagDesc : CComponent::DESC
	{
		CollType eCollType{};
		_float3 vCenter{};
		_float3 vExtents{};
		_float4 quatOritented{};
		_float fRadius{};
		_float4x4 matFrustum{};
		StringID CollCastHint;
	}DESC;
public:
	DECLARE_DERIVED_TYPE(CComCollider, CComponent)

public:
	void UpdateGUI() override;

private:
	explicit CComCollider();
	~CComCollider() override;

private:
	HRESULT Initialize(void* pArg) override;
	HRESULT InitializeCollider(const DESC* pDesc);

public:
	//void LocalTransform(_fmatrix mat);
	void Transform(_fmatrix mat);

public:
	CCollider* Get() const { return m_pCollider.get(); }

private:
	SPtr<CCollider> m_pCollider{};
	_float3 m_vLocalOffset{};


public:
	static UPtr<CComCollider> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END