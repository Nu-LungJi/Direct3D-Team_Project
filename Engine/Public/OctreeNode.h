#pragma once
#include "Engine_Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL COctreeNode : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(COctreeNode, CEngineBase)

protected:
	COctreeNode();
	COctreeNode(const COctreeNode& Prototype);
	~COctreeNode() override;

public:
	HRESULT Initialize(const BoundingBox& bounds, uint32_t depth, uint32_t maxDepth);
	void BuildOctree(const std::vector<CHandle>& hObjects);

	bool IsLeaf() const;

	HRESULT RenderDebugOctree();
	void SetDebugDrawOctree(_bool draw) { m_bDebugDrawOctree = draw; }

private:
	void Subdivide();

private:
	BoundingBox m_bounds;
	std::vector<CHandle> m_hObjects;
	std::array<UPtr<COctreeNode>, 8> m_childrenNode;
	uint32_t m_depth = 0;
	uint32_t m_maxDepth = 4;

private:
	_bool m_bDebugDrawOctree = false;
};

NS_END