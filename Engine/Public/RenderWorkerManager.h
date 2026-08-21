#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CRenderWorkerManager final : public CEngineBase
{
private:
	struct RENDER_WORKER
	{
		std::thread thread{};
		_string sTaskName{};
	};

	struct RENDER_WORKER_TASK
	{
		_string sTaskName{};
		std::function<void(ID3D11DeviceContext*)> func{};
	};

private:
	explicit CRenderWorkerManager(const std::string& sName);
	~CRenderWorkerManager() override;

public:
	void UpdateGUI();
	uint32_t GetWorkerCount() const { return static_cast<uint32_t>(m_Workers.size()); }

	_bool Enqueue(_string_view svTaskName, std::function<void(ID3D11DeviceContext*)> func)
	{
		if (!func)
		{
			return false;
		}

		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			if (m_bStop)
			{
				return false;
			}

			RENDER_WORKER_TASK task{};
			task.sTaskName = svTaskName;
			task.func = std::move(func);
			m_Tasks.push_back(std::move(task));
		}

		m_Condition.notify_one();
		return true;
	}

	template<typename Func>
	auto EnqueueWithFuture(_string_view svTaskName, Func&& f)
		-> std::future<std::invoke_result_t<Func, ID3D11DeviceContext*>>
	{
		using ReturnType = std::invoke_result_t<Func, ID3D11DeviceContext*>;

		auto task = std::make_shared<std::packaged_task<ReturnType(ID3D11DeviceContext*)>>(
			std::forward<Func>(f));
		auto future = task->get_future();

		Enqueue(svTaskName,
			[task](ID3D11DeviceContext* pDeferredContext)
			{
				(*task)(pDeferredContext);
			});
		return future;
	}

private:
	HRESULT Initialize(uint32_t iThreadCount);
	void ShutDown();

private:
	std::vector<RENDER_WORKER> m_Workers{};
	std::vector<ComPtr<ID3D11DeviceContext>> m_pDeferredContexts{};
	std::list<RENDER_WORKER_TASK> m_Tasks{};
	std::mutex m_Mutex{};
	std::condition_variable m_Condition{};
	_bool m_bStop = false;
	std::string m_sName{};

public:
	static UPtr<CRenderWorkerManager> Create(const std::string& sName, uint32_t iThreadCount);
};

NS_END
