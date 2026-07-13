#pragma once
#include "Engine_Base.h"

NS_BEGIN(Engine)

struct OCTREE_DEBUG_BOUNDS
{
	BoundingBox bounds{};
	uint32_t depth = 0;
	FXMVECTOR color = Colors::Lime;
};

class ENGINE_DLL COctreeNode : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(COctreeNode, CEngineBase)

protected:
	COctreeNode();
	COctreeNode(const COctreeNode& Prototype);
	~COctreeNode() override;

public:
	const std::vector<CHandle>& GetObjectHandles() { return m_hObjects; }

public:
	HRESULT Initialize(const BoundingBox& bounds, uint32_t depth, uint32_t maxDepth);
	void BuildOctree(const std::vector<CHandle>& hObjects);
	void CollectDebugBounds(std::vector<OCTREE_DEBUG_BOUNDS>& outBounds) const;
	void OctreeFrustumCull(const BoundingFrustum& cameraFrustum);
	void CollectRayCandidates(FXMVECTOR rayOrigin, FXMVECTOR rayDirection, std::vector<CHandle>& outHandles) const;

	bool IsLeaf() const;

	const BoundingBox& GetBoundingBox() const { return m_bounds; }
	const BoundingBox& GetCullingBoundingBox() const { return m_cullingBounds; }
	HRESULT RenderDebugOctree();
	void SetDebugDrawOctree(_bool draw) { m_bDebugDrawOctree = draw; }

private:
	void Subdivide();
	void RebuildCullingBounds();

private:
	BoundingBox m_bounds; // 공간 분할용
	BoundingBox m_cullingBounds; // 프러스텀 컬링용
	std::vector<CHandle> m_hObjects;
	std::array<UPtr<COctreeNode>, 8> m_childrenNode;
	uint32_t m_depth = 0;
	uint32_t m_maxDepth = 4;

private:
	_bool m_bDebugDrawOctree = false;
	_bool m_bInCameraFrustum = false; // 디버그렌더용

public:
	static UPtr<COctreeNode> Create(const BoundingBox& bounds, uint32_t depth, uint32_t maxDepth = 4);
};

NS_END
