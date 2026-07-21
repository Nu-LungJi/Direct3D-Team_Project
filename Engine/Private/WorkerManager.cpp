#include "pch.h"
#include "WorkerManager.h"

NS_USING(Engine)

CWorkerManager::CWorkerManager(const std::string& sName)
    : m_sName{sName}
{
}

CWorkerManager::~CWorkerManager()
{
    ShutDown();
}

void CWorkerManager::UpdateGUI()
{
    std::string s = m_sName + "_Workers";
    ImGui::Begin(s.c_str());

    //if (ImGui::Button("TEST1"))
    //{
    //    Enqueue("TEST_SLEEP1", []() {std::this_thread::sleep_for(std::chrono::milliseconds(2000)); });
    //}

    //if (ImGui::Button("TEST2"))
    //{
    //    m_TestFuture = WorkerEnqueueWithFuture("TEST_SLEEP2", []() {
    //        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    //        thread_local std::mt19937 gen(std::random_device{}());
    //        std::uniform_int_distribution<int> dist(0, 100);

    //        return  dist(gen);
    //        });
    //}


    //if (m_TestFuture.valid())
    //{
    //    if (m_TestFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    //    {
    //        m_iTest = m_TestFuture.get();
    //        //auto asdf = future.get();
    //    }
    //}

    //ImGui::Text("Test: %i", m_iTest);

    if (ImGui::TreeNode("Workers"))
    {
		std::vector<std::string> taskNames;
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			taskNames.reserve(m_Workers.size());
			for (const auto& worker : m_Workers)
			{
				taskNames.push_back(worker.sTaskName);
			}
		}

		for (size_t i = 0; i < taskNames.size(); ++i)
        {
			const char* taskName = taskNames[i].empty() ? "IDLE" : taskNames[i].c_str();
			ImGui::Text("Worker#%zu: %s", i, taskName);
        }
        ImGui::TreePop();
    }


    {
        std::list<std::string> taskNames{};
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            for (const auto& task : m_Tasks)
            {
                taskNames.push_back(task.sTaskName);
            }
        }

        if (ImGui::TreeNode("PendingTasks"))
        {
            ImGui::Text("Size: %i", taskNames.size());
            int i{};
            for (const auto& taskName : taskNames)
            {
                ImGui::Text("PendingTask_%i: %s", i, taskName.c_str());
                ++i;
            }
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

HRESULT CWorkerManager::Initialize(uint32_t iThreadCount)
{
	if (iThreadCount == 0)
	{
		iThreadCount = 1;
	}

	// Worker storage must be stable before any worker thread can access it.
	m_Workers.resize(iThreadCount);

    for (uint32_t i = 0; i < iThreadCount; ++i)
    {
		m_Workers[i].thread = std::thread([this, i]()
            {
                char threadName[32];
                sprintf_s(threadName, "Worker-%d", i);
                tracy::SetThreadName(threadName);

                while (true)
                {
                    WORKER_TASK task{};
                    {
                        std::unique_lock<std::mutex> lock(m_Mutex);
                        /*
                            조건에 따라 스레드를 Sleep 시켜서 cpu안태움
                            Sleep시에 자동으로 락을 해제 해준다.

                            Pred == true : Sleep을 풀고 락을 유지한채로 스레드를 러닝한다.
                            Pred == false: Sleep을 유지하고 락을 풀어준다.

                            m_Condition의  notify_one() 또는 notify_all()를 호출하면
                            Sleep중인 스레드가 깨고
                            다시 조건을 체크해서 sleep할지 다음으로 넘어갈지 판단한다.
                        */
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
                    }
					{
						std::lock_guard<std::mutex> lock(m_Mutex);
						m_Workers[i].sTaskName = task.sTaskName;
					}
                    {
                        ZoneScopedN("WorkerTaskExecution");
                        if (!task.sTaskName.empty()) {
                            TracyMessage(task.sTaskName.c_str(), task.sTaskName.size());
                        }
						try
						{
							task.func();
						}
						catch (const std::exception& e)
						{
							std::string message = "Worker task failed [" + task.sTaskName + "]: " + e.what() + "\n";
							OutputDebugStringA(message.c_str());
						}
						catch (...)
						{
							std::string message = "Worker task failed [" + task.sTaskName + "]: unknown exception\n";
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

void CWorkerManager::ShutDown()
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
}

UPtr<CWorkerManager> CWorkerManager::Create(const std::string& sName, uint32_t iThreadCount)
{
    auto pInstance = UPtr<CWorkerManager>(new CWorkerManager{ sName });

    if (FAILED(pInstance->Initialize(iThreadCount)))
    {
        MSG_BOX("CWorkerManager Create Failed");
        return nullptr;
    }

    return pInstance;
}
