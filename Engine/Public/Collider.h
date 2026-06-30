#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCollider: public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CCollider, CEngineBase)

public:
	enum class CollType { Box,OrientedBox,Sphere,Frustum };
	CollType GetCollType() const { return m_eCollType; }

public:
	CEngineBase* GetInnerPointer() const { return m_pInner; }
	void SetInnerPointer(CEngineBase* p) { m_pInner = p ; }
	StringID GetInnerHint() const { return m_InnerHint; }
	void SetInnerHint(StringID hint) { m_InnerHint = hint; }
	std::shared_ptr<void> GetInnerHint2() const { return m_pInnerHint2; }
	//void SetInnerHint2(void* p, size_t size) {
	//	if (m_pInnerHint2) free(m_pInnerHint2);
	//	m_pInnerHint2 = malloc(size);
	//	m_InnerHint2Size = size;
	//	memcpy(m_pInnerHint2, p, size);
	//}
	void SetInnerHint2(std::shared_ptr<void> p)
	{
		m_pInnerHint2 = p;
	}
	//_bool UpdateInnerHint2(void* p, size_t size)
	//{
	//	m_pInnerHint2.get();
	//	//if (m_InnerHint2Size != size)
	//	//{
	//	//	return false;
	//	//}

	//	//memcpy(m_pInnerHint2, p, size);
	//	return true;
	//}
	

public:
	void SetOriginalColor(const _float4& color) { m_vOriginalColor = color; }
	void SetIntersectColor(const _float4& color) { m_vIntersectColor = color; }
	const _float4& GetOriginalColor() const { return m_vOriginalColor; }
	const _float4& GetIntersectColor() const { return m_vIntersectColor; }

protected:
	explicit CCollider(CollType eCollType);
	~CCollider() override;

public:
	virtual _bool Intersect(const CCollider& collider) const = 0;
	virtual void Transform(_fmatrix wordMatrix) = 0;

protected:
	const CollType m_eCollType{};
	CEngineBase* m_pInner{};
	StringID m_InnerHint{};
	std::shared_ptr<void> m_pInnerHint2{};
	//size_t m_InnerHint2Size{};

protected:
	_float4 m_vOriginalColor{ Colors::Green };
	_float4 m_vIntersectColor{ Colors::Red };

private:
	void Free() override;
};

NS_END