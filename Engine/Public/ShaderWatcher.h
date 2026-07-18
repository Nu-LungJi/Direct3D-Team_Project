#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CShaderWatcher final : public CEngineBase
{
private:
	CShaderWatcher();
	~CShaderWatcher() override;

public:
	std::vector<_string> GetChangedFilesAndClear();

private:
	HRESULT Initialize();
	void StartWatching(const std::wstring& path);

private:
	std::vector<_string> m_ChangedFiles{};
	std::mutex m_Mutex{};
	std::atomic_bool m_bIsRunning{ true };
	std::vector<HANDLE> m_DirectoryHandles{};
	std::vector<std::thread> m_WatcherThreads{};

public:
	static UPtr<CShaderWatcher> Create();

private:
	void Free() override;
};

NS_END
