#pragma once

#include "AnimatedObjectPlacementData.h"

NS_BEGIN(Engine)

struct ANIMATED_OBJECT_PLACEMENT_RESULT
{
	uint64_t iPlacementId{};
	_bool bSucceeded{};
	CHandle hObject{};
	_string sMessage{};
};

class ENGINE_DLL CAnimatedObjectPlacementManager final : public CEngineBase
{
public:
	using SPAWN_CALLBACK = std::function<ANIMATED_OBJECT_PLACEMENT_RESULT(const ANIMATED_OBJECT_PLACEMENT_DESC&)>;

private:
	CAnimatedObjectPlacementManager() = default;
	~CAnimatedObjectPlacementManager() = default;

public:
	void UpdateGUI();
	uint64_t AddPlacement(const ANIMATED_OBJECT_PLACEMENT_DESC& Desc = {});
	_bool RemovePlacement(uint64_t iPlacementId);
	void ClearPlacements();
	void RegisterOption(const _string& sDisplayName, const ANIMATED_OBJECT_PLACEMENT_DESC& Desc,
		const std::vector<_string>& AnimationNames = {});
	void ClearOptions();
	void SetSpawnCallback(SPAWN_CALLBACK Callback) { m_SpawnCallback = std::move(Callback); }
	void ClearSpawnCallback() { m_SpawnCallback = {}; }
	void SetPickingQueryMask(uint32_t iQueryMask) { m_iPickingQueryMask = iQueryMask; }
	ANIMATED_OBJECT_PLACEMENT_RESULT Spawn(uint64_t iPlacementId);
	const std::vector<ANIMATED_OBJECT_PLACEMENT_RESULT>& SpawnAll();
	HRESULT Save(const _string& sFilePath) const;
	HRESULT Load(const _string& sFilePath);
	const std::vector<ANIMATED_OBJECT_PLACEMENT_DESC>& GetPlacements() const { return m_Placements; }

private:
	struct OPTION
	{
		_string sDisplayName{};
		ANIMATED_OBJECT_PLACEMENT_DESC Desc{};
		std::vector<_string> AnimationNames{};
	};
	void DrawEditor(ANIMATED_OBJECT_PLACEMENT_DESC& Desc);
	void UpdatePicking();
	_string Validate(const ANIMATED_OBJECT_PLACEMENT_DESC& Desc) const;
	uint64_t AllocateId();

private:
	std::vector<ANIMATED_OBJECT_PLACEMENT_DESC> m_Placements{};
	std::vector<ANIMATED_OBJECT_PLACEMENT_RESULT> m_LastResults{};
	std::vector<OPTION> m_Options{};
	SPAWN_CALLBACK m_SpawnCallback{};
	uint64_t m_iNextPlacementId{ 1 };
	int32_t m_iSelectedIndex{ -1 };
	_string m_sFilePath{ "./Resources/json/AnimatedObject/Level_HogwartWorld.json" };
	_string m_sFileStatus{};
	_bool m_bPlacementPicking{};
	_bool m_bSpawnOnPick{ true };
	uint32_t m_iPickingQueryMask{ PX_ALL_LAYERS };

public:
	static UPtr<CAnimatedObjectPlacementManager> Create();
};

NS_END
