#include "pch.h"
#include "RenderWorkerManager.h"

NS_USING(Engine)

CRenderWorkerManager::CRenderWorkerManager(const std::string& sName)
	: m_sName{ sName }
{
}

CRenderWorkerManager::~CRenderWorkerManager()
{
	ShutDown();
}

void CRenderWorkerManager::UpdateGUI()
{
	const std::string windowName = m_sName + "_Workers";
	ImGui::Begin(windowName.c_str());

	std::vector<std::string> workerTaskNames{};
	std::vector<std::string> pendingTaskNames{};
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		workerTaskNames.reserve(m_Workers.size());
		for (const auto& worker : m_Workers)
		{
			workerTaskNames.push_back(worker.sTaskName);
		}
		pendingTaskNames.reserve(m_Tasks.size());
		for (const auto& task : m_Tasks)
		{
			pendingTaskNames.push_back(task.sTaskName);
		}
	}

	if (ImGui::TreeNode("Workers"))
	{
		for (size_t i = 0; i < workerTaskNames.size(); ++i)
		{
			const char* taskName = workerTaskNames[i].empty() ? "IDLE" : workerTaskNames[i].c_str();
			ImGui::Text("RenderWorker#%zu: %s", i, taskName);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("PendingTasks"))
	{
		ImGui::Text("Size: %zu", pendingTaskNames.size());
		for (size_t i = 0; i < pendingTaskNames.size(); ++i)
		{
			ImGui::Text("PendingTask_%zu: %s", i, pendingTaskNames[i].c_str());
		}
		ImGui::TreePop();
	}

	ImGui::End();
}

HRESULT CRenderWorkerManager::Initialize(uint32_t iThreadCount)
{
	if (iThreadCount == 0)
	{
		iThreadCount = 1;
	}

	m_Workers.resize(iThreadCount);
	m_pDeferredContexts.resize(iThreadCount);

	const ComPtr<ID3D11Device> pDevice = CGameInstance::Get().GetGraphicDevice();
	if (pDevice == nullptr)
	{
		return E_FAIL;
	}

	// 1스레드당 1디퍼드컨텍스트 생성
	for (uint32_t i = 0; i < iThreadCount; ++i)
	{
		if (FAILED(pDevice->CreateDeferredContext(0, m_pDeferredContexts[i].GetAddressOf())))
		{
			return E_FAIL;
		}
	}

	for (uint32_t i = 0; i < iThreadCount; ++i)
	{
		m_Workers[i].thread = std::thread([this, i]()
			{
				char threadName[32]{};
				sprintf_s(threadName, "%s-%u", m_sName.c_str(), i);
				tracy::SetThreadName(threadName);

				while (true)
				{
					RENDER_WORKER_TASK task{};
					{
						std::unique_lock<std::mutex> lock(m_Mutex);
						m_Condition.wait(lock, [this]()
							{
								return m_bStop || !m_Tasks.empty();
							});

						if (m_bStop && m_Tasks.empty())
						{
							return;
						}

						task = std::move(m_Tasks.front());
						m_Tasks.pop_front();
						m_Workers[i].sTaskName = task.sTaskName;
					}

					{
						ZoneScopedN("RenderWorkerTaskExecution");
						if (!task.sTaskName.empty())
						{
							TracyMessage(task.sTaskName.c_str(), task.sTaskName.size());
						}

						try
						{
							task.func(m_pDeferredContexts[i].Get());
						}
						catch (const std::exception& e)
						{
							const std::string message = "Render worker task failed [" +
								task.sTaskName + "]: " + e.what() + "\n";
							OutputDebugStringA(message.c_str());
						}
						catch (...)
						{
							const std::string message = "Render worker task failed [" +
								task.sTaskName + "]: unknown exception\n";
							OutputDebugStringA(message.c_str());
						}
					}

					{
						std::lock_guard<std::mutex> lock(m_Mutex);
						m_Workers[i].sTaskName.clear();
					}
				}
			});
	}

	return S_OK;
}

void CRenderWorkerManager::ShutDown()
{
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_bStop = true;
	}
	m_Condition.notify_all();

	for (auto& worker : m_Workers)
	{
		if (worker.thread.joinable())
		{
			worker.thread.join();
		}
	}

	m_Workers.clear();
	m_pDeferredContexts.clear();
}

UPtr<CRenderWorkerManager> CRenderWorkerManager::Create(const std::string& sName, uint32_t iThreadCount)
{
	auto pInstance = UPtr<CRenderWorkerManager>(new CRenderWorkerManager{ sName });
	if (FAILED(pInstance->Initialize(iThreadCount)))
	{
		MSG_BOX("CRenderWorkerManager Create Failed");
		return nullptr;
	}
	return pInstance;
}
