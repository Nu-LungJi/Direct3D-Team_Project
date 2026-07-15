#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CLuaWatcher final : public CEngineBase
{
private:
	CLuaWatcher();
	~CLuaWatcher();

public:
	void UpdateGUI();
	// 메인 스레드에서 변경된 파일 목록을 안전하게 가져오는 함수
	std::vector<std::string> GetChangedFilesAndClear();

private:
	HRESULT Initialize();
	void StartWatching(const std::wstring& path);

private:
	std::vector<std::string> m_changedFiles;
	std::mutex m_mtx;
	//std::thread m_watcherThread;
	std::atomic<bool> m_bIsRunning;
	//HANDLE m_hDir = INVALID_HANDLE_VALUE;

	std::vector<HANDLE> m_hDirs;
	std::vector<std::thread> m_watcherThreads;
public:
	static UPtr<CLuaWatcher> Create();

private:
	void Free() override;
};
NS_END
