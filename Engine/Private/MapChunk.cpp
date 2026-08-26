#include "pch.h"
#include "MapChunk.h"
#include "OctreeNode.h"

NS_USING(Engine)

CMapChunk::CMapChunk() = default;

CMapChunk::CMapChunk(const MAPCHUNK_COORD& coord, const BoundingBox& bounds)
	: m_Coord{ coord }
	, m_Bounds{ bounds }
{
}

CMapChunk::~CMapChunk() = default;
CMapChunk::CMapChunk(CMapChunk&&) noexcept = default;
CMapChunk& CMapChunk::operator=(CMapChunk&&) noexcept = default;

void CMapChunk::BeginLoading()
{
	ClearObjects();
	m_LoadState = EChunkLoadState::Loading;
}

void CMapChunk::CompleteLoading(const BoundingBox& bounds, EChunkSaveState saveState)
{
	m_Bounds = bounds;
	m_SaveState = saveState;
	RebuildOctree();
	m_LoadState = EChunkLoadState::Loaded;
}

void CMapChunk::CancelLoading()
{
	ClearObjects();
	m_LoadState = EChunkLoadState::Unloaded;
}

void CMapChunk::BeginUnloading()
{
	m_LoadState = EChunkLoadState::Unloading;
}

void CMapChunk::CompleteUnloading()
{
	ClearObjects();
	m_LoadState = EChunkLoadState::Unloaded;
}

_bool CMapChunk::AddObject(const CHandle& objectHandle)
{
	if (ContainsObject(objectHandle))
		return false;

	m_ObjectHandles.push_back(objectHandle);

	return true;
}

_bool CMapChunk::RemoveObject(const CHandle& objectHandle)
{
	const auto iter = std::find(m_ObjectHandles.begin(), m_ObjectHandles.end(), objectHandle);
	if (iter == m_ObjectHandles.end())
		return false;

	m_ObjectHandles.erase(iter);

	return true;
}

void CMapChunk::ClearObjects()
{
	m_ObjectHandles.clear();
	m_pOctree.reset();
}

_bool CMapChunk::ContainsObject(const CHandle& objectHandle) const
{
	return std::find(m_ObjectHandles.begin(), m_ObjectHandles.end(), objectHandle) != m_ObjectHandles.end();
}

void CMapChunk::RebuildOctree()
{
	m_pOctree = COctreeNode::Create(m_Bounds, 0);

	if (m_pOctree)
		m_pOctree->BuildOctree(m_ObjectHandles);
}

std::vector<MAP_MODEL_RESOURCE_KEY> CMapChunk::TakeModelResources()
{
	auto resources = std::move(m_ModelResources);
	m_ModelResources.clear();

	return resources;
}

const BoundingBox& CMapChunk::GetCullingBounds() const
{
	return m_pOctree ? m_pOctree->GetCullingBoundingBox() : m_Bounds;
}

_bool CMapChunk::CanAutoLoad() const
{
	return m_LoadState == EChunkLoadState::Unloaded && m_SaveState != EChunkSaveState::Unsaved;
}

_bool CMapChunk::CanAutoUnload() const
{
	return m_LoadState == EChunkLoadState::Loaded && m_SaveState != EChunkSaveState::Unsaved;
}
