#pragma once
#include "Engine_Base.h"
#include <functional>

NS_BEGIN(Engine)

class COctreeNode;

// 맵 청크 좌표를 unordered 컨테이너의 키로 사용할 때 필요한 해시 함수다.
struct tagMapChunkCoordHash
{
	size_t operator()(const MAPCHUNK_COORD& coord) const
	{
		const size_t xHash = std::hash<int64_t>{}(coord.x);
		const size_t yHash = std::hash<int64_t>{}(coord.y);
		const size_t zHash = std::hash<int64_t>{}(coord.z);
		return xHash ^ (yHash << 1) ^ (zHash << 3);
	}
};

enum class EChunkLoadState
{
	Unloading,  // 언로드 요청 중
	Unloaded,   // 메타데이터만 있고 월드에는 존재하지 않음
	Loading,    // 비동기 또는 동기 로드가 진행 중
	Loaded,     // 게임 오브젝트와 런타임 데이터가 월드에 반영됨
};

enum class EChunkSaveState
{
	Unsaved,    // 아직 저장 파일이 없거나 편집으로 내용이 변경됨
	Saved,      // 저장 파일과 현재 청크 내용이 일치함
};

// 청크가 참조하는 정적 모델 리소스를 식별한다.
struct MAP_MODEL_RESOURCE_KEY
{
	std::string group;
	std::string tag;

	bool operator==(const MAP_MODEL_RESOURCE_KEY& rhs) const
	{
		return group == rhs.group && tag == rhs.tag;
	}
};

struct MAP_MODEL_RESOURCE_KEY_HASH
{
	size_t operator()(const MAP_MODEL_RESOURCE_KEY& key) const
	{
		const size_t groupHash = std::hash<std::string>{}(key.group);
		const size_t tagHash = std::hash<std::string>{}(key.tag);
		return groupHash ^ (tagHash << 1);
	}
};

// 하나의 맵 청크가 소유하는 런타임 오브젝트와 공간 분할 정보,
// 로드·저장 상태를 일관된 상태로 관리한다.
class ENGINE_DLL CMapChunk final
{
public:
	CMapChunk();
	CMapChunk(const MAPCHUNK_COORD& coord, const BoundingBox& bounds);
	~CMapChunk();

	CMapChunk(const CMapChunk&) = delete;
	CMapChunk& operator=(const CMapChunk&) = delete;
	CMapChunk(CMapChunk&&) noexcept;
	CMapChunk& operator=(CMapChunk&&) noexcept;

public:
	// 청크의 고정 좌표와 공간 경계를 갱신한다.
	void SetCoord(const MAPCHUNK_COORD& coord) { m_Coord = coord; }
	void SetBounds(const BoundingBox& bounds) { m_Bounds = bounds; }

	// 스트리밍 파일 경로와 저장 상태는 MapManager의 저장 정책에 따라 변경한다.
	void SetFilePath(std::string filePath) { m_FilePath = std::move(filePath); }
	void SetSaveState(EChunkSaveState state) { m_SaveState = state; }

	// 로드가 시작되기 전에 이전 런타임 오브젝트와 옥트리를 비운다.
	void BeginLoading();
	// 생성된 오브젝트로 옥트리를 만들고 Loaded 상태를 확정한다.
	void CompleteLoading(const BoundingBox& bounds, EChunkSaveState saveState);
	// 로드 실패 또는 취소 시 런타임 데이터를 제거하고 Unloaded 상태로 복구한다.
	void CancelLoading();

	// 언로드 처리 중임을 표시한다. 실제 오브젝트 제거 요청은 MapManager가 수행한다.
	void BeginUnloading();
	// 런타임 오브젝트와 옥트리를 제거하고 Unloaded 상태를 확정한다.
	void CompleteUnloading();

	// 청크에 속한 게임 오브젝트 핸들을 중복 없이 추가한다.
	_bool AddObject(const CHandle& objectHandle);
	_bool RemoveObject(const CHandle& objectHandle);
	void ClearObjects();
	_bool ContainsObject(const CHandle& objectHandle) const;
	void RebuildOctree();

	// 모델 리소스 참조 목록은 MapManager의 참조 카운트 관리에서 사용한다.
	std::vector<MAP_MODEL_RESOURCE_KEY>& GetModelResources() { return m_ModelResources; }
	const std::vector<MAP_MODEL_RESOURCE_KEY>& GetModelResources() const { return m_ModelResources; }
	void SetModelResources(std::vector<MAP_MODEL_RESOURCE_KEY> resources) { m_ModelResources = std::move(resources); }
	std::vector<MAP_MODEL_RESOURCE_KEY> TakeModelResources();

public:
	const MAPCHUNK_COORD& GetCoord() const { return m_Coord; }
	const std::vector<CHandle>& GetObjectHandles() const { return m_ObjectHandles; }
	const BoundingBox& GetBounds() const { return m_Bounds; }
	const BoundingBox& GetCullingBounds() const;
	const COctreeNode* GetOctree() const { return m_pOctree.get(); }
	COctreeNode* GetOctree() { return m_pOctree.get(); }
	EChunkLoadState GetLoadState() const { return m_LoadState; }
	EChunkSaveState GetSaveState() const { return m_SaveState; }
	const std::string& GetFilePath() const { return m_FilePath; }

	_bool IsLoaded() const { return m_LoadState == EChunkLoadState::Loaded; }
	_bool CanAutoLoad() const;
	_bool CanAutoUnload() const;

private:
	MAPCHUNK_COORD m_Coord{};
	std::vector<CHandle> m_ObjectHandles{};
	std::vector<MAP_MODEL_RESOURCE_KEY> m_ModelResources{};
	BoundingBox m_Bounds{};
	UPtr<COctreeNode> m_pOctree{};

	EChunkLoadState m_LoadState = EChunkLoadState::Unloaded;
	EChunkSaveState m_SaveState = EChunkSaveState::Unsaved;
	std::string m_FilePath{};
};

NS_END
