#include "pch.h"
#include "OctreeNode.h"

NS_USING(Engine)

COctreeNode::COctreeNode()
{
}

COctreeNode::COctreeNode(const COctreeNode& Prototype)
{
}

COctreeNode::~COctreeNode()
{
}

HRESULT COctreeNode::Initialize(const BoundingBox& bounds, uint32_t depth, uint32_t maxDepth)
{
	m_bounds = bounds;
	m_depth = depth;
	m_maxDepth = maxDepth;

	m_hObjects.clear();
	for (auto& child : m_childrenNode)
	{
		child.reset();
	}

	return S_OK;
}

void COctreeNode::BuildOctree(const std::vector<CHandle>& hObjects)
{
	m_hObjects.clear();

	std::vector<CHandle> containedObjects;
	containedObjects.reserve(hObjects.size());

	// hObjects들 중 내 BoundingBox안에 들어와있는 애들만 갖고 있는다.
	for (const auto& handle : hObjects)
	{
		CGameObject* pObj = CGameInstance::Get().GetGameObjectByHandle(handle);
		if (pObj == nullptr)
			continue;

		const _float3& pos = pObj->GetTransform().GetPosition();
		const _vector vPos = XMVectorSet(pos.x, pos.y, pos.z, 1.f);
		if (m_bounds.Contains(vPos) != DirectX::DISJOINT)
		{
			containedObjects.push_back(handle);
		}
	}

	// 내 BoundingBox안에 있는 오브젝트가 없으면 리턴
	if (containedObjects.empty())
		return;

	// 분할 (내 bound 8분할 해서 자식들 boundingBox에 적용)
	Subdivide();
	
	for (const auto& childNode : m_childrenNode)
	{
		childNode->BuildOctree(containedObjects);
	}
}

bool COctreeNode::IsLeaf() const
{
	for (const auto& child : m_childrenNode)
	{
		if (child)
			return false;
	}

	return true;
}

HRESULT COctreeNode::RenderDebugOctree()
{


	return S_OK;
}

void COctreeNode::Subdivide()
{
	_float3 myCenterpos = m_bounds.Center;
	_float3 myExtents = m_bounds.Extents;

	const _float3 childExtents =
	{
		myCenterpos.x * 0.5f,
		myCenterpos.y * 0.5f,
		myCenterpos.z * 0.5f
	};

	size_t index = 0;

	for (int z = -1; z <= 1; z += 2)
	{
		for (int y = -1; y <= 1; y += 2)
		{
			for (int x = -1; x <= 1; x += 2)
			{
				const _float3 childCenter =
				{
					myCenterpos.x + childExtents.x * static_cast<float>(x),
					myCenterpos.y + childExtents.y * static_cast<float>(y),
					myCenterpos.z + childExtents.z * static_cast<float>(z)
				};

				if (m_childrenNode[index] == nullptr)
				{
					m_childrenNode[index] = ToUPtr(new COctreeNode{});
				}

				m_childrenNode[index]->Initialize(
					BoundingBox(childCenter, childExtents),
					m_depth + 1,
					m_maxDepth
				);

				++index;
			}
		}
	}
}
