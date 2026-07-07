#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CWorkerManager final : public CEngineBase
{
private:
	typedef struct tagWorker
	{
		std::thread thread;
		_string sTaskName;
	} WORKER;

private:
	CWorkerManager(const std::string& sName);
	~CWorkerManager();

public:
	void UpdateGUI();

public:
	void Enqueue(_string_view svTaskName, _Func func)
	{
		{
			std::lock_guard<std::mutex> lock(m_Mutex);

			WORKER_TASK task{};
			task.sTaskName = svTaskName;
			task.func = func;
			m_Tasks.push_back(task);
		}
		m_Condition.notify_one();
	}

	template<typename Func, typename... Args>
	auto WorkerEnqueueWithFuture(_string_view svTaskName, Func&& f, Args&&... args)
		-> std::future<std::invoke_result_t<Func, Args...>>
	{
		using ReturnType = std::invoke_result_t <Func, Args...>;

		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
		);

		std::future<ReturnType> future = task->get_future();
		Enqueue(svTaskName, [task]() {(*task)(); });
		return future;
	}

private:
	HRESULT Initialize(uint32_t iThreadCount = std::thread::hardware_concurrency());
	void ShutDown();

private:
	std::vector<WORKER> m_Workers{}; 
	std::list<WORKER_TASK> m_Tasks{}; 
	std::mutex m_Mutex{};
	std::condition_variable m_Condition{};

	_bool m_bStop{ false };

	int m_iTest{};
	std::future<int> m_TestFuture{};

private:
	std::string m_sName{};

public:
	static UPtr<CWorkerManager> Create(const std::string& sName, uint32_t iThreadCount);
};

NS_END
