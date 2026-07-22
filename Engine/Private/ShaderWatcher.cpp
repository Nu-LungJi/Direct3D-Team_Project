#include "pch.h"
#include "ShaderWatcher.h"

NS_USING(Engine)

CShaderWatcher::CShaderWatcher() = default;
CShaderWatcher::~CShaderWatcher() = default;

HRESULT CShaderWatcher::Initialize()
{
	// Executables use each project Bin directory as their working directory.
	StartWatching(L"../ShaderFiles");
	StartWatching(L"../../Engine/ShaderFiles");
	return S_OK;
}

void CShaderWatcher::StartWatching(const std::wstring& path)
{
	HANDLE hDirectory = CreateFileW(
		path.c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr);

	if (hDirectory == INVALID_HANDLE_VALUE)
		return;

	m_DirectoryHandles.push_back(hDirectory);
	m_WatcherThreads.emplace_back([this, path, hDirectory]()
	{
		std::array<std::byte, 4096> buffer{};
		DWORD bytesReturned{};

		while (m_bIsRunning.load(std::memory_order_acquire))
		{
			const BOOL succeeded = ReadDirectoryChangesW(
				hDirectory,
				buffer.data(),
				static_cast<DWORD>(buffer.size()),
				TRUE,
				FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
				&bytesReturned,
				nullptr,
				nullptr);

			if (!succeeded || bytesReturned == 0)
				continue;

			auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());
			for (;;)
			{
				std::filesystem::path relativePath{
					std::wstring{ info->FileName, info->FileNameLength / sizeof(wchar_t) } };
				_string extension = WStringToString(relativePath.extension().wstring());
				std::ranges::transform(extension, extension.begin(),
					[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

				if (extension == ".hlsl" || extension == ".hlsli")
				{
					_string changedPath = WStringToString((std::filesystem::path{ path } / relativePath).wstring());
					std::replace(changedPath.begin(), changedPath.end(), '\\', '/');

					std::lock_guard lock{ m_Mutex };
					if (std::ranges::find(m_ChangedFiles, changedPath) == m_ChangedFiles.end())
						m_ChangedFiles.push_back(std::move(changedPath));
				}

				if (info->NextEntryOffset == 0)
					break;
				info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
					reinterpret_cast<std::byte*>(info) + info->NextEntryOffset);
			}
		}
	});
}

std::vector<_string> CShaderWatcher::GetChangedFilesAndClear()
{
	std::lock_guard lock{ m_Mutex };
	std::vector<_string> changedFiles{};
	changedFiles.swap(m_ChangedFiles);
	return changedFiles;
}

UPtr<CShaderWatcher> CShaderWatcher::Create()
{
	auto instance = UPtr<CShaderWatcher>{ new CShaderWatcher{} };
	if (FAILED(instance->Initialize()))
		return nullptr;
	return instance;
}

void CShaderWatcher::Free()
{
	m_bIsRunning.store(false, std::memory_order_release);

	for (HANDLE hDirectory : m_DirectoryHandles)
	{
		if (hDirectory != INVALID_HANDLE_VALUE)
		{
			CancelIoEx(hDirectory, nullptr);
			CloseHandle(hDirectory);
		}
	}
	m_DirectoryHandles.clear();

	for (auto& thread : m_WatcherThreads)
		if (thread.joinable())
			thread.join();
	m_WatcherThreads.clear();

	CEngineBase::Free();
}
