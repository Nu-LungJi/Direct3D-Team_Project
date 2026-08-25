#pragma once

#include "Engine_Base.h"
#include "MapChunk.h"
#include "MapChunkSerializer.h"
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

NS_BEGIN(Engine)

// 스트리밍 청크가 사용하는 정적 모델의 로드와 참조 수명을 관리한다.
// 모델별 로드 락, 워커의 임시 참조, 로드된 청크의 참조, 지연 해제를 한곳에서 처리한다.
class ENGINE_DLL CMapModelResourceTracker final
{
public:
	using CHUNK_MAP =
		std::unordered_map<MAPCHUNK_COORD, CMapChunk, tagMapChunkCoordHash>;

public:
	// 모델 태그로 실제 .bin 파일을 찾을 수 있도록 맵의 모델 인덱스를 교체한다.
	void SetResourceIndex(
		const std::filesystem::path& staticModelRoot,
		const std::string& resourceGroup,
		std::unordered_map<std::string, std::filesystem::path> modelPaths);

	// 동기 청크 로드에 필요한 모델을 확보하고 청크 참조 수를 증가시킨다.
	HRESULT AcquireChunkResources(
		CMapChunk& chunk,
		const std::vector<MAP_MESH_OBJECT_FILE_DATA>& objects);

	// 워커 스레드에서 모델을 미리 로드하고 월드 반영 전 임시 참조를 등록한다.
	HRESULT PreloadResources(
		const std::vector<MAP_MESH_OBJECT_FILE_DATA>& objects,
		std::vector<MAP_MODEL_RESOURCE_KEY>& outResources);

	// 워커의 임시 참조를 로드 완료 청크의 참조로 전환한다.
	HRESULT CommitPreloadedResources(
		CMapChunk& chunk,
		const std::vector<MAP_MODEL_RESOURCE_KEY>& resources);

	// 취소되거나 실패한 비동기 로드의 임시 참조를 반환한다.
	void ReleasePreloadedResources(
		const std::vector<MAP_MODEL_RESOURCE_KEY>& resources);

	// 청크가 보유한 모델 참조를 다음 메인 스레드 처리 시점에 해제하도록 예약한다.
	void QueueChunkRelease(CMapChunk& chunk);
	void QueueAllChunkReleases(CHUNK_MAP& chunks);
	void ProcessDeferredReleases();

private:
	using MODEL_KEY_SET =
		std::unordered_set<MAP_MODEL_RESOURCE_KEY, MAP_MODEL_RESOURCE_KEY_HASH>;
	using MODEL_REF_COUNT_MAP =
		std::unordered_map<MAP_MODEL_RESOURCE_KEY, size_t, MAP_MODEL_RESOURCE_KEY_HASH>;

private:
	MODEL_KEY_SET CollectUniqueModelKeys(
		const std::vector<MAP_MESH_OBJECT_FILE_DATA>& objects) const;
	HRESULT EnsureResourceLoaded(const MAP_MODEL_RESOURCE_KEY& key);
	SPtr<std::mutex> GetResourceMutex(const MAP_MODEL_RESOURCE_KEY& key);
	void DecreaseChunkReferences(
		const std::vector<MAP_MODEL_RESOURCE_KEY>& resources,
		std::vector<MAP_MODEL_RESOURCE_KEY>& outUnusedCandidates);
	void DecreasePendingReferences(
		const std::vector<MAP_MODEL_RESOURCE_KEY>& resources);

private:
	// 현재 맵에서 모델 태그를 실제 파일 경로로 변환하기 위한 인덱스다.
	std::filesystem::path m_StaticModelRoot;
	std::string m_ResourceGroup;
	std::unordered_map<std::string, std::filesystem::path> m_ModelPaths;
	std::mutex m_ResourceIndexMutex;

	// 동일 모델을 여러 워커가 동시에 로드하거나 해제하지 못하게 하는 모델별 락이다.
	std::mutex m_ResourceMutexMapMutex;
	std::unordered_map<MAP_MODEL_RESOURCE_KEY, SPtr<std::mutex>, MAP_MODEL_RESOURCE_KEY_HASH>
		m_ResourceMutexes;

	// 월드 반영 전 워커 결과가 모델을 붙잡고 있는 횟수다.
	std::mutex m_PendingReferenceMutex;
	MODEL_REF_COUNT_MAP m_PendingReferenceCounts;

	// 완전히 로드된 청크들이 각 모델을 사용 중인 횟수다. 메인 스레드에서만 변경한다.
	MODEL_REF_COUNT_MAP m_ChunkReferenceCounts;

	// 청크 언로드와 로드 실패에서 발생한 해제를 안전한 메인 스레드 시점까지 보관한다.
	std::vector<std::vector<MAP_MODEL_RESOURCE_KEY>> m_DeferredChunkReleases;
	std::vector<MAP_MODEL_RESOURCE_KEY> m_DeferredUnusedCandidates;
};

NS_END
