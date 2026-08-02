#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CLevel;
class CLevelManager final : public CEngineBase
{
private:
	CLevelManager();
	~CLevelManager();

public:
	void UpdateGUI();
	void RegisterLevelChangeFunc(const _string& ID, _Func func)
	{
		m_LevelChangeFuncs.emplace(ID, func);
	};

public:
	HRESULT ChangeLevel(const _string& ID);
	HRESULT ChangeLevel(UPtr<CLevel> pNewLevel);
	uint32_t GetCurrentLevelID() const;
	void Update(_float fTimeDelta);
	HRESULT Render();

	void FrameStart(_float fTimeDelta);
	void FrameEnd(_float fTimeDelta);

private:
	std::string m_sChangeLevel{};
	UPtr<CLevel> m_pCurrentLevel{};

	UPtr<CLevel> m_pLevelBeforeLevelChange{};
	CLevel* m_pLevelBeingInitialized{ nullptr };

	std::map<_string, _Func> m_LevelChangeFuncs{};

public:
	static UPtr<CLevelManager> Create();
};

NS_END
