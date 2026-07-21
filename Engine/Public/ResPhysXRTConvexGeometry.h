#pragma once
#include "ResPhysXConvexGeometry.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResPhysXRTConvexGeometry final : public CResPhysXConvexGeometry
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
	DECLARE_DERIVED_TYPE(CResPhysXRTConvexGeometry, CResPhysXConvexGeometry)

private:
	explicit CResPhysXRTConvexGeometry(const _string& sPath);
	~CResPhysXRTConvexGeometry() override;

public:
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
	static HRESULT CookToMemory(const DESC& desc, std::vector<uint8_t>& outCookedData);
	static HRESULT CookToFile(const DESC& desc, const _string& sOutputPath);

public:
	static SPtr<CResPhysXRTConvexGeometry> Create();
	static SPtr<CResPhysXRTConvexGeometry> CreateAndLoad(const DESC& desc);
};

NS_END
