#pragma once
#include "Engine_Defines.h"
#include "GameObject.h"
#include "Handle.h"
#include "Slot.h"
#include <span>
#include <unordered_set>

NS_BEGIN(Engine)

class ENGINE_DLL CGameObjectManager final : public CEngineBase
{
	// GameInstance의 private bridge를 통해서만 GameObject의 삭제 요청을 전달받는다.
	friend class CGameInstance;
public:
	CGameObjectManager(const CGameObjectManager&) = delete;
	CGameObjectManager& operator=(const CGameObjectManager& rhs) = delete;

private:
	CGameObjectManager();
	~CGameObjectManager() override;

private:
	HRESULT Initialize();

public:
	void UpdateGUI();
private:
	char m_GUISearchFilter[256] = {};
	_bool m_bGUIEnableSearchInput{ false };
	_bool m_bGUIShowInvalidLayerHandles{ false };
	_bool m_bTracyDetailedObjectProfiling{ false };
	bool MatchesGUIFilter(std::string_view sText) const;
	bool MatchesLayerObjectFilter(std::string_view sLayerName, CGameObject* pObj) const;
	std::string GetGameObjectDebugLabel(CGameObject* pObj) const;
	std::string GetInvalidLayerHandleDebugText(const CHandle& handle) const;

public:
	void FrameStart();
	void FrameEnd();

private:
	std::optional<CHandle> AllocateHandleSlot();

public:
	CGameObject* GetGameObjectByHandle(const CHandle& handle) { return const_cast<CGameObject*>(_GetGameObjectByHandle(handle)); }
	const CGameObject* GetGameObjectByHandle(const CHandle& handle) const { return _GetGameObjectByHandle(handle); }
	template<typename T> T* GetGameObjectByHandleT(const CHandle& handle);
	template<typename T> const T* GetGameObjectByHandleT(const CHandle& handle) const;
private:
	const CGameObject* _GetGameObjectByHandle(const CHandle& handle ) const;

public:
	std::optional<CHandle> AddGameObjectToLayer(const StringID& siProtoGroupTag, const StringID& siPrototypeTag, std::string_view sLayerName, void* pArg);
	const std::vector<CHandle>* GetLayer(std::string_view sLayerName) const;
	const std::vector<CHandle>* GetLayer(std::string_view sLayerName, const StringID& iPrototypeLevelIndex, const StringID& svPrototypeTag, void* pArg) ;
	const std::vector<std::pair<std::string, std::vector<CHandle>>>& GetLayers() const { return m_Layers; }
	template<typename T> T* GetFirstGameObjectByLayer(std::string_view sLayerName);
	void DelLayer(std::string_view sLayerName);

public:
	void AllReset();
	size_t ResetObjectsInLayers(std::span<const std::string_view> layerNames);
	size_t ResetAllObjectsExceptLayers(std::span<const std::string_view> excludedLayerNames);

private:
	enum class RESET_LAYER_MODE : uint8_t
	{
		INCLUDE_ONLY,
		EXCLUDE
	};

	// [LSY] 레이어 이름과 모드에 따라 일괄 제거할 오브젝트를 PendingDestroy 상태로 예약한다.
	// INCLUDE_ONLY는 전달한 레이어만 제거하고, EXCLUDE는 전달한 레이어를 제외한 나머지를 제거한다.
	// 존재하지 않는 레이어 이름은 무시하며, 반환값은 이번 호출에서 새로 제거 예약된 오브젝트 수다.
	// 실제 오브젝트 해제와 레이어 컨테이너 정리는 안전한 FrameEnd 시점에 일괄 처리한다.
	size_t RequestResetByLayers(
		std::span<const std::string_view> layerNames,
		RESET_LAYER_MODE eMode);
	void QueuePendingDestroy(const CHandle& hObject);
	_bool DestroyQueuedPendingObjects();
	_bool RemoveEmptyResetTargetLayers(
		const std::unordered_set<std::string>& resetTargetLayers);

	_bool m_bBatchResetPending{ false };
	std::unordered_set<std::string> m_PendingResetTargetLayers{};

	// SetPendingDestroy가 쓰는 요청 큐와 현재 파괴 중인 큐를 분리한다.
	// 따라서 객체 소멸자에서 새 파괴 요청이 발생해도 순회 중인 메모리를 건드리지 않고
	// 같은 FrameEnd while 루프의 다음 묶음으로 넘길 수 있다.
	std::vector<CHandle> m_PendingDestroyHandles{};
	std::vector<CHandle> m_ProcessingDestroyHandles{};

public:
	void FixedUpdate(_float fScaledDelta, _float fUnscaledDelta);
	void PriorityUpdate(_float fScaledDelta, _float fUnscaledDelta);
	void Update(_float fScaledDelta, _float fUnscaledDelta);
	void LateUpdate(_float fScaledDelta, _float fUnscaledDelta);

private:
	// m_Objects, m_ObjectLayerLocations, m_PendingLayerRemoveGenerationKeys는
	// Handle의 slot index를 동일한 인덱스로 사용하는 병렬 저장소다.
	// 슬롯이 늘거나 재사용될 때 항상 함께 초기화하여 별도 해시 조회 없이 레이어를 찾는다.
	// Manager에 등록된 객체 하나는 정확히 한 레이어의 Handle 배열에만 존재한다는 불변식을 전제로 한다.
	std::vector<CSlot<CGameObject>> m_Objects{};
	std::vector<size_t> m_FreeSlots{};

	struct OBJECT_LAYER_LOCATION
	{
		static constexpr size_t INVALID_INDEX =
			std::numeric_limits<size_t>::max();
		static constexpr uint32_t INVALID_GENERATION =
			std::numeric_limits<uint32_t>::max();

		size_t iLayerIndex{ INVALID_INDEX };
		size_t iHandleIndex{ INVALID_INDEX };
		// 재사용된 같은 slot index를 과거 Handle의 위치 정보와 구분한다.
		uint32_t iGeneration{ INVALID_GENERATION };

		_bool Matches(const CHandle& hObject) const
		{
			return iLayerIndex != INVALID_INDEX &&
				iHandleIndex != INVALID_INDEX &&
				iGeneration == hObject.GetGeneration();
		}
	};

	// slot index -> (m_Layers index, 레이어 내부 Handle index, generation)
	std::vector<OBJECT_LAYER_LOCATION> m_ObjectLayerLocations{};

	// 레이어 안정 압축 중 제거할 slot을 표시하는 일회성 scratch다.
	// 0은 미예약, 나머지는 generation + 1로 저장한다. uint64_t에서 더하므로
	// uint32_t의 UINT_MAX generation도 0 sentinel과 충돌하지 않는다.
	std::vector<uint64_t> m_PendingLayerRemoveGenerationKeys{};
	// 한 삭제 묶음에서 실제로 압축해야 하는 레이어 인덱스만 중복 제거해 보관한다.
	std::vector<size_t> m_AffectedLayerIndices{};

private:
	struct StringHash {
		using is_transparent = void;  // 이게 핵심

		size_t operator()(std::string_view sv) const {
			return std::hash<std::string_view>{}(sv);
		}
	};

	struct StringEqual {
		using is_transparent = void;  // 이것도 필요

		bool operator()(std::string_view a, std::string_view b) const {
			return a == b;
		}
	};

	std::vector<std::pair<std::string, std::vector<CHandle>>> m_Layers{};
	std::unordered_map<std::string, size_t, StringHash, StringEqual> m_LookupLayers{};
	// 외부 레이어 순서가 바뀌는 구조 변경 시 lookup과 모든 slot 위치표를 함께 재구축한다.
	void SortLayer();
	void SetObjectLayerLocation(
		const CHandle& hObject,
		size_t iLayerIndex,
		size_t iHandleIndex);
	_bool FindObjectLayerIndex(
		const CHandle& hObject,
		size_t& iOutLayerIndex) const;
	void RemovePendingObjectsFromLayers();

private:
	// 정렬된 레이어와 각 레이어의 Handle 삽입 순서를 마스크로 안정 필터링한 비소유 포인터 view다.
	// NONE 객체는 네 view 어디에도 들어가지 않고, 복수 비트 객체는 여러 view에 들어간다.
	// 객체 수명은 FrameEnd 지연 파괴로 보장하고, 런타임 상태인 PendingDestroy와
	// ManagedUpdateEnabled는 후보 배열을 다시 만들지 않고 각 dispatch 직전에 확인한다.
	// 연속적인 것은 포인터 목록이며 실제 GameObject 메모리 배치까지 연속화하는 구조는 아니다.
	_bool m_bUpdateViewsDirty{ true };
	std::vector<CGameObject*> m_PriorityUpdateObjects{};
	std::vector<CGameObject*> m_FixedUpdateObjects{};
	std::vector<CGameObject*> m_UpdateObjects{};
	std::vector<CGameObject*> m_LateUpdateObjects{};


public:
	static UPtr< CGameObjectManager> Create();

public:
	void Free() override;
};

NS_END

inline const Engine::CGameObject* Engine::CGameObjectManager::_GetGameObjectByHandle(const CHandle& handle) const
{
	size_t idx = handle.GetIndex();
	if (idx >= m_Objects.size())
	{
		return nullptr;
	}

	const auto& slot = m_Objects[idx];
	if (!slot.IsOccupied())
	{
		return nullptr;
	}
	if (slot.GetGeneration() != handle.GetGeneration())
	{
		return nullptr;
	}

	return slot.Get();
}

template<typename T>
inline T* Engine::CGameObjectManager::GetGameObjectByHandleT(const CHandle& handle)
{
	const CGameObject* obj = _GetGameObjectByHandle(handle);
	return const_cast<T*>(Engine::Cast<T>(obj));
}


template<typename T>
inline const T* Engine::CGameObjectManager::GetGameObjectByHandleT(const CHandle& handle) const
{
	const CGameObject* obj = _GetGameObjectByHandle(handle);
	return Engine::Cast<T>(obj);
}

template<typename T>
inline T* Engine::CGameObjectManager::GetFirstGameObjectByLayer(std::string_view sLayerName)
{
	auto* pLayer = GetLayer(sLayerName);
	if (pLayer->empty())
	{
		return nullptr;
	}
	T* pObj = GetGameObjectByHandleT<T>(pLayer->front());
	if (!pObj)
	{
		return nullptr;
	}
	return pObj;
}
