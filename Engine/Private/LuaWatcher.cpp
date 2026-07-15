#include "pch.h"
#include "LuaWatcher.h"

NS_USING(Engine)

CLuaWatcher::CLuaWatcher() : m_bIsRunning(true) {}

CLuaWatcher::~CLuaWatcher()
{
	//m_bIsRunning = false;
	//if (m_watcherThread.joinable()) m_watcherThread.join();
}

HRESULT CLuaWatcher::Initialize()
{
	// 리소스 폴더 경로 설정 (실제 프로젝트 경로에 맞게 수정 필요)
	StartWatching(L"../LuaFiles");
	StartWatching(L"../../Engine/LuaFiles");
	return S_OK;
}

void CLuaWatcher::StartWatching(const std::wstring& path)
{
	// 1. 핸들 생성
	HANDLE hDir = CreateFileW(path.c_str(), FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	if (hDir == INVALID_HANDLE_VALUE) return;

	// 2. 벡터에 핸들 저장 (나중에 Free에서 종료하기 위함)
	m_hDirs.push_back(hDir);

	// 3. 스레드를 벡터에 추가 (emplace_back)
	m_watcherThreads.emplace_back([this, path, hDir]() {
		char buffer[1024];
		DWORD bytesReturned;

		// 루프 내부에서는 캡처받은 지역변수 hDir을 사용
		while (m_bIsRunning) {
			if (ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), TRUE,
				FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME, &bytesReturned, NULL, NULL)) {

				FILE_NOTIFY_INFORMATION* pInfo = (FILE_NOTIFY_INFORMATION*)buffer;
				do {
					std::wstring fileName(pInfo->FileName, pInfo->FileNameLength / sizeof(WCHAR));

					if (fileName.length() >= 4 && fileName.substr(fileName.length() - 4) == L".lua")
					{
						// 절대 경로로 조합 (path + fileName)
						std::filesystem::path fullPath = std::filesystem::path(path) / fileName;
						std::string fileNameStr{ WStringToString(fullPath.wstring()) };
						std::replace(fileNameStr.begin(), fileNameStr.end(), '\\', '/');

						std::lock_guard<std::mutex> lock(m_mtx);
						if (std::find(m_changedFiles.begin(), m_changedFiles.end(), fileNameStr) == m_changedFiles.end()) {
							m_changedFiles.push_back(fileNameStr);
						}
					}
					if (pInfo->NextEntryOffset == 0) break;
					pInfo = (FILE_NOTIFY_INFORMATION*)((char*)pInfo + pInfo->NextEntryOffset);
				} while (true);
			}
		}
		});
}

std::vector<std::string> CLuaWatcher::GetChangedFilesAndClear()
{
	std::lock_guard<std::mutex> lock(m_mtx);
	std::vector<std::string> temp = m_changedFiles;
	m_changedFiles.clear();
	return temp;
}

UPtr<CLuaWatcher> CLuaWatcher::Create()
{
	auto pInstance = UPtr<CLuaWatcher>(new CLuaWatcher{});
	if (FAILED(pInstance->Initialize())) return nullptr;
	return pInstance;
}

void CLuaWatcher::Free()
{
	m_bIsRunning = false;

	// 모든 핸들의 입출력을 취소하고 닫음
	for (HANDLE hDir : m_hDirs) {
		if (hDir != INVALID_HANDLE_VALUE) {
			CancelIoEx(hDir, NULL);
			CloseHandle(hDir);
		}
	}
	m_hDirs.clear();

	// 모든 스레드 종료 대기
	for (auto& t : m_watcherThreads) {
		if (t.joinable()) {
			t.join();
		}
	}
	m_watcherThreads.clear();

	CEngineBase::Free();
}

void CLuaWatcher::UpdateGUI() { /* GUI 구현부 */ }
