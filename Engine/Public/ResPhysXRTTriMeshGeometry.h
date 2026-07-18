#pragma once
#include "ResPhysXGeometry.h"

NS_BEGIN(physx)
class PxTriangleMesh;
NS_END

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXRTTriMeshGeometry final : public CResPhysXGeometry
{
public:
	enum class INDEX_FORMAT : uint8_t
	{
		UINT16,
		UINT32
	};

	struct DESC
	{
		const void* pVertexData{};
		uint32_t iVertexCount{};
		uint32_t iVertexStride{};
		uint32_t iPositionOffset{};

		const void* pIndexData{};
		uint32_t iIndexCount{};
		INDEX_FORMAT eIndexFormat{ INDEX_FORMAT::UINT32 };
	};
public:
	DECLARE_DERIVED_TYPE(CResPhysXRTTriMeshGeometry, CResPhysXGeometry)

private:
	explicit CResPhysXRTTriMeshGeometry(const _string& sPath);
	~CResPhysXRTTriMeshGeometry() override;

public:
	physx::PxTriangleMesh* GetTriMesh() const { return m_pTriMesh; }

	template<typename TVertex, typename TIndex>
	static DESC MakeDesc(
		const std::vector<TVertex>& vertices,
		const std::vector<TIndex>& indices,
		size_t iPositionOffset)
	{
		static_assert(std::is_same_v<TIndex, uint16_t> || std::is_same_v<TIndex, uint32_t>,
			"TriMesh indices must be uint16_t or uint32_t.");

		DESC desc{};
		desc.pVertexData = vertices.data();
		desc.iVertexCount = static_cast<uint32_t>(vertices.size());
		desc.iVertexStride = sizeof(TVertex);
		desc.iPositionOffset = static_cast<uint32_t>(iPositionOffset);
		desc.pIndexData = indices.data();
		desc.iIndexCount = static_cast<uint32_t>(indices.size());
		desc.eIndexFormat = std::is_same_v<TIndex, uint16_t>
			? INDEX_FORMAT::UINT16
			: INDEX_FORMAT::UINT32;
		return desc;
	}

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	physx::PxTriangleMesh* m_pTriMesh{};
public:
	static SPtr<CResPhysXRTTriMeshGeometry> Create();

private:
	void Free() override;
};

NS_END
