#include "pch.h"
#include "OctreeNode.h"
#include "MapMeshObject.h"

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

	// 노드에 오브젝트 없으면 리턴
	if (containedObjects.empty())
		return;

	if (m_depth >= m_maxDepth /*|| containedObjects.size() <= 8*/)
	{
		m_hObjects = std::move(containedObjects);
		return;
	}

	// 8개 자식 바운딩박스 생성
	Subdivide();
	
	for (const auto& childNode : m_childrenNode)
	{
		if (childNode)
			childNode->BuildOctree(containedObjects);
	}
}

void COctreeNode::CollectDebugBounds(std::vector<OCTREE_DEBUG_BOUNDS>& outBounds) const
{
	outBounds.push_back(OCTREE_DEBUG_BOUNDS
		{
			.bounds = m_bounds,
			.depth = m_depth,
			.color = m_bInCameraFrustum ? Colors::Magenta : Colors::Cyan
		});

	for (const auto& childNode : m_childrenNode)
	{
		if (childNode)
			childNode->CollectDebugBounds(outBounds);
	}
}

void COctreeNode::OctreeFrustumCull(const BoundingFrustum& cameraFrustum)
{
	// 카메라 프러스텀과 교차안하면 그냥 리턴 	// 교차 안하면 암것도안함 (고려대상에서 버림)
	if (!m_bounds.Intersects(cameraFrustum))
	{
		m_bInCameraFrustum = false; //디버그 렌더용
		return;
	}

	// 카메라 프러스텀과 교차한다면
	{
		m_bInCameraFrustum = true; //디버그 렌더용
		if (m_depth >= m_maxDepth) // 리프노드라면
		{
			for (const auto& handle : m_hObjects)
			{
				CMapMeshObject* mapObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);
				if (mapObj == nullptr)
					continue;

				mapObj->SetRenderEnable(true);
			}
			return;
		}

		for (const auto& child : m_childrenNode)
		{
			if (child)
				child->OctreeFrustumCull(cameraFrustum);
		}
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
		myExtents.x * 0.5f,
		myExtents.y * 0.5f,
		myExtents.z * 0.5f
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

// OctreeNode.cpp
UPtr<COctreeNode> COctreeNode::Create(const BoundingBox& bounds, uint32_t depth, uint32_t maxDepth)
{
	auto pInstance = ToUPtr(new COctreeNode{});
	if (FAILED(pInstance->Initialize(bounds, depth, maxDepth)))
	{
		return nullptr;
	}

	return pInstance;
}
