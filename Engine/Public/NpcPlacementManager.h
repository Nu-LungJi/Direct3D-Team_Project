#pragma once

#include <array>
#include "NpcPlacementData.h"

NS_BEGIN(Engine)

struct NPC_PLACEMENT_RESULT
{
	uint64_t iPlacementId{};
	_bool bSucceeded{};
	CHandle hObject{};
	_string sMessage{};
};

/*
 * NPC 생성 정책
 * 1. Prototype 등록은 Client Loader가 한 번만 수행한다.
 * 2. 이 매니저는 NPC를 직접 new/Clone하거나 소유하지 않는다.
 * 3. Client SPAWN_CALLBACK은 NPC 전용 GAMEOBJECT_DESC를 구성한 뒤
 *    CGameInstance::AddGameObjectToLayer를 통해 생성한다.
 * 4. 런타임 개체는 raw pointer가 아닌 CHandle로 추적한다.
 */

class ENGINE_DLL CNpcPlacementManager final : public CEngineBase
{
public:
	using SPAWN_CALLBACK = std::function<NPC_PLACEMENT_RESULT(const NPC_PLACEMENT_DESC&)>;

private:
	enum class FILE_CONFIRM_ACTION : uint8_t
	{
		NONE,
		SAVE,
		LOAD
	};

private:
	CNpcPlacementManager();
	~CNpcPlacementManager();

public:
	void UpdateGUI();

	uint64_t AddPlacement(const NPC_PLACEMENT_DESC& Desc = {});
	_bool RemovePlacement(uint64_t iPlacementId);
	void ClearPlacements();

	const std::vector<NPC_PLACEMENT_DESC>& GetPlacements() const { return m_Placements; }
	const std::vector<NPC_PLACEMENT_RESULT>& GetLastResults() const { return m_LastResults; }

	void SetSpawnCallback(SPAWN_CALLBACK Callback) { m_SpawnCallback = std::move(Callback); }
	void ClearSpawnCallback() { m_SpawnCallback = {}; }
	_bool HasSpawnCallback() const { return static_cast<_bool>(m_SpawnCallback); }
	void RegisterNpcOption(const _string& sDisplayName, const NPC_PLACEMENT_DESC& Desc);
	void RegisterNpcSkeletonOption(const _string& sPrototypeTag, const _string& sDisplayName,
		const _string& sModelGroupTag, const _string& sModelResourceTag, const _string& sAnimResourcePath = {});
	void RegisterBehaviorOption(const _string& sDisplayName,
		const _string& sBehaviorMajorTag, const _string& sBehaviorMinorTag);
	void ClearNpcOptions();
	void SetPickingQueryMask(uint32_t iQueryMask) { m_iPickingQueryMask = iQueryMask; }
	void AddMinorNameToNpcPlacement(const _string& strName) { m_ResMinorNames.push_back(strName); }
	NPC_PLACEMENT_RESULT Spawn(uint64_t iPlacementId);
	const std::vector<NPC_PLACEMENT_RESULT>& SpawnAll();
	HRESULT Save(const _string& sFilePath) const;
	HRESULT Load(const _string& sFilePath);

private:
	inline static constexpr std::array<const char*, 3> RUNTIME_TYPE_NAMES
	{
		"CPU Actor - Combat",
		"CPU Actor - Ambient",
		"GPU Crowd - Ambient"
	};

	static void DrawStringInput(const char* pLabel, _string& Value);
	static _bool DrawStringSelection(const char* pLabel, _string& Value, const std::vector<_string>& Options);
	static void SortAndUnique(std::vector<_string>& Options);
	void DrawPlacementEditor(NPC_PLACEMENT_DESC& Desc, size_t iIndex);
	void DrawFileConfirmPopup();
	void UpdatePlacementPicking();
	void SyncPlacement(const NPC_PLACEMENT_DESC& Desc);
	void DestroyRuntimeObject(const CHandle& Handle);
	void ClearRuntimeObjects();
	static _bool IsSamePlacement(const NPC_PLACEMENT_DESC& Left, const NPC_PLACEMENT_DESC& Right);
	NPC_PLACEMENT_RESULT SpawnPlacement(const NPC_PLACEMENT_DESC& Desc) const;
	_string ValidatePlacement(const NPC_PLACEMENT_DESC& Desc) const;
	uint64_t AllocatePlacementId();
	std::vector<_string> RegistAnimPath(const _string& sAnimPath);
private:
	std::vector<NPC_PLACEMENT_DESC> m_Placements{};
	std::vector<NPC_PLACEMENT_RESULT> m_LastResults{};
	std::unordered_map<uint64_t, CHandle> m_RuntimeObjects{};
	std::unordered_map<uint64_t, NPC_PLACEMENT_DESC> m_RuntimeDescs{};
	std::vector<std::pair<_string, NPC_PLACEMENT_DESC>> m_NpcOptions{};
	struct NPC_SKELETON_OPTION
	{
		_string sPrototypeTag{};
		_string sDisplayName{};
		_string sModelGroupTag{};
		_string sModelResourceTag{};
		std::vector<_string> sAnimPath{};
	};
	std::vector<NPC_SKELETON_OPTION> m_NpcSkeletonOptions{};
	struct BEHAVIOR_OPTION
	{
		_string sDisplayName{};
		_string sBehaviorMajorTag{};
		_string sBehaviorMinorTag{};
	};
	std::vector<BEHAVIOR_OPTION> m_BehaviorOptions{};
	SPAWN_CALLBACK m_SpawnCallback{};
	uint64_t m_iNextPlacementId{ 1 };
	int32_t m_iSelectedIndex{ -1 };
	_string m_sFilePath{ "./Resources/json/NPC/Level_HogwartWorld.json" };
	_string m_sFileStatus{};
	_bool m_bPlacementPicking{};
	std::vector<_string>		m_ResMinorNames;
	uint32_t m_iPickingQueryMask{ PX_ALL_LAYERS };
	FILE_CONFIRM_ACTION m_eFileConfirmAction{ FILE_CONFIRM_ACTION::NONE };

public:
	static UPtr<CNpcPlacementManager> Create();
};

NS_END
