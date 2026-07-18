#pragma once
#include "ResPhysXGeometry.h"

NS_BEGIN(physx)
class PxConvexMesh;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXRTConvexGeometry final : public CResPhysXGeometry
{
public:
	struct DESC
	{
		const void* pVertexData{};
		uint32_t iVertexCount{};
		uint32_t iVertexStride{};
		uint32_t iPositionOffset{};
		uint16_t iVertexLimit{ 255 };
	};

public:
	DECLARE_DERIVED_TYPE(CResPhysXRTConvexGeometry, CResPhysXGeometry)

private:
	explicit CResPhysXRTConvexGeometry(const _string& sPath);
	~CResPhysXRTConvexGeometry() override;

public:
	physx::PxConvexMesh* GetConvexMesh() const { return m_pConvexMesh; }

	template<typename TVertex>
	static DESC MakeDesc(
		const std::vector<TVertex>& vertices,
		size_t iPositionOffset,
		uint16_t iVertexLimit = 255)
	{
		DESC desc{};
		desc.pVertexData = vertices.data();
		desc.iVertexCount = static_cast<uint32_t>(vertices.size());
		desc.iVertexStride = sizeof(TVertex);
		desc.iPositionOffset = static_cast<uint32_t>(iPositionOffset);
		desc.iVertexLimit = iVertexLimit;
		return desc;
	}

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	physx::PxConvexMesh* m_pConvexMesh{};

public:
	static SPtr<CResPhysXRTConvexGeometry> Create();

private:
	void Free() override;
};

NS_END
