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
	m_cullingBounds = m_bounds;
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
	m_cullingBounds = m_bounds;

	for (auto& child : m_childrenNode)
		child.reset();
	
	if (hObjects.empty())
		return;

	if (m_depth >= m_maxDepth)
	{
		for (const CHandle& handle : hObjects)
		{
			CMapMeshObject* object =CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);
			if (object != nullptr)
				m_hObjects.push_back(handle);
		}
		RebuildCullingBounds();
		return;
	}

	// 8개 자식 바운딩박스 생성
	Subdivide();

	// 다음 깊이로 내려보낼 오브젝트들
	std::array<std::vector<CHandle>,8> childBuckets;

	for (const auto& handle : hObjects)
	{
		CMapMeshObject* pObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);
		if (pObj == nullptr)
			continue;
		pObj->GetTransform().Update();

		BoundingBox objBox{};
		if (!pObj->GetOcclusionBounds(objBox))
		{
			m_hObjects.push_back(handle);
			continue;
		}
		
		bool assigned = false;

		for (size_t i = 0; i < childBuckets.size(); ++i)
		{
			if (auto& childNode = m_childrenNode[i])
			{
				if (childNode->GetBoundingBox().Contains(objBox) == DirectX::CONTAINS)
				{
					childBuckets[i].push_back(handle);
					assigned = true;
					break;
				}
			}
		}


		if (assigned == false)
		{
			m_hObjects.push_back(handle);
		}
	}

	for (size_t i = 0; i < m_childrenNode.size(); ++i)
	{
		if (childBuckets[i].empty())
		{
			m_childrenNode[i].reset();
			continue;
		}

		m_childrenNode[i]->BuildOctree(childBuckets[i]);
	}

	RebuildCullingBounds();
}

void COctreeNode::CollectDebugBounds(std::vector<OCTREE_DEBUG_BOUNDS>& outBounds) const
{
	outBounds.push_back(OCTREE_DEBUG_BOUNDS
		{
			.bounds = m_cullingBounds,
			.depth = m_depth,
			// CPU 프러스텀 디버그 색상 비활성화
			//.color = m_bInCameraFrustum ? Colors::Magenta : Colors::Cyan
			.color = Colors::Cyan
		});

	for (const auto& childNode : m_childrenNode)
	{
		if (childNode)
			childNode->CollectDebugBounds(outBounds);
	}
}

//void COctreeNode::OctreeFrustumCull(const BoundingFrustum& cameraFrustum)
//{
//	const ContainmentType containment = cameraFrustum.Contains(m_cullingBounds);
//
//	// 교차안하면 바로 리턴
//	if (containment == DirectX::DISJOINT)
//	{
//		m_bInCameraFrustum = false; //디버그 렌더용
//		return;
//	}
//	if (containment == DirectX::CONTAINS)
//	{
//		m_bInCameraFrustum = true;
//		SetAllObjectsVisibleRecursive();
//		return;
//	}
//
//	// 옥트리 노드랑 프러스텀 걸쳐있을 때
//	// 카메라 프러스텀과 교차한다면
//	{
//		m_bInCameraFrustum = true; //디버그 렌더용
//		//if (m_depth >= m_maxDepth) // 리프노드라면
//		//{
//		for (const auto& handle : m_hObjects)
//		{
//			CMapMeshObject* mapObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);
//			if (mapObj == nullptr)
//				continue;
//
//			BoundingBox objBox{};
//			if (!mapObj->GetOcclusionBounds(objBox) || objBox.Intersects(cameraFrustum))
//				mapObj->SetRenderEnable(true);
//		}
//		//return;
//	//}
//
//		for (const auto& child : m_childrenNode)
//		{
//			if (child)
//				child->OctreeFrustumCull(cameraFrustum);
//		}
//	}
//}
//
//
void COctreeNode::CollectRayCandidates(FXMVECTOR rayOrigin, FXMVECTOR rayDirection, std::vector<CHandle>& outHandles) const
{
	_float nodeDistance = 0.f;
	if (!m_cullingBounds.Intersects(rayOrigin, rayDirection, nodeDistance))
		return;

	for (const CHandle& handle : m_hObjects)
	{
		CMapMeshObject* mapObject = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);
		if (mapObject == nullptr)
			continue;

		BoundingBox objectBounds{};
		_float objectDistance = 0.f;
		if (mapObject->GetOcclusionBounds(objectBounds) &&
			objectBounds.Intersects(rayOrigin, rayDirection, objectDistance))
		{
			outHandles.push_back(handle);
		}
	}

	for (const auto& child : m_childrenNode)
	{
		if (child)
			child->CollectRayCandidates(rayOrigin, rayDirection, outHandles);
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

void COctreeNode::RebuildCullingBounds()
{
	m_cullingBounds = m_bounds;

	for (const CHandle& handle : m_hObjects)
	{
		CMapMeshObject* mapObject =
			CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);
		if (mapObject == nullptr)
			continue;

		BoundingBox objectBounds{};
		if (!mapObject->GetOcclusionBounds(objectBounds))
			continue;

		BoundingBox::CreateMerged(m_cullingBounds, m_cullingBounds, objectBounds);
	}

	for (const auto& childNode : m_childrenNode)
	{
		if (childNode == nullptr)
			continue;

		BoundingBox::CreateMerged(m_cullingBounds, m_cullingBounds, childNode->m_cullingBounds);
	}
}

//void COctreeNode::SetAllObjectsVisibleRecursive()
//{
//	for (auto& myObjHandle : m_hObjects)
//	{
//		CMapMeshObject* myObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(myObjHandle);
//		if (myObj)
//		{
//			myObj->SetRenderEnable(true);
//		}
//	}
//
//	for (auto& myChild : m_childrenNode)
//	{
//		if (myChild)
//		{
//			myChild->SetAllObjectsVisibleRecursive();
//		}
//	}
//}
//
//
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
