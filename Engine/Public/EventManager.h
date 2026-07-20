#pragma once
#include "Engine_Base.h"

#include <typeindex>

NS_BEGIN(Engine)

using EVENT_LISTENER_ID = uint64_t;

class ENGINE_DLL CEventManager final : public CEngineBase
{
public:
	CEventManager(const CEventManager&) = delete;
	CEventManager& operator=(const CEventManager&) = delete;

private:
	struct LISTENER
	{
		EVENT_LISTENER_ID id = 0;
		CHandle owner;
		std::function<void(const void*)> callback;
	};

	struct QUEUED_EVENT
	{
		std::type_index type;
		std::shared_ptr<const void> payload;
	};

private:
	CEventManager();
	~CEventManager() override;

	HRESULT Initialize();

public:
	template<typename TEvent, typename TCallback>
	EVENT_LISTENER_ID Subscribe(CHandle owner, TCallback&& callback)
	{
		using CALLBACK_TYPE = std::decay_t<TCallback>;
		static_assert(
			std::is_invocable_v<CALLBACK_TYPE, const TEvent&> ||
			std::is_invocable_v<CALLBACK_TYPE>,
			"Event callback must be callable with const TEvent& or with no arguments.");

		LISTENER listener{};
		listener.id = m_NextListenerId++;
		listener.owner = owner;

		if constexpr (std::is_invocable_v<CALLBACK_TYPE, const TEvent&>)
		{
			listener.callback = [func = std::forward<TCallback>(callback)](const void* payload)
				{
					func(*static_cast<const TEvent*>(payload));
				};
		}
		else
		{
			listener.callback = [func = std::forward<TCallback>(callback)](const void*)
				{
					func();
				};
		}

		const EVENT_LISTENER_ID listenerId = listener.id;
		m_Listeners[typeid(TEvent)].emplace_back(std::move(listener));
		return listenerId;
	}

	// 특정 콜백 하나만 해제
	template<typename TEvent>
	void Unsubscribe(EVENT_LISTENER_ID listenerId)
	{
		auto iter = m_Listeners.find(typeid(TEvent));
		if (iter == m_Listeners.end())
			return;

		auto& listeners = iter->second;

		std::erase_if(
			listeners,
			[listenerId](const LISTENER& listener)
			{
				return listener.id == listenerId;
			}
		);

		if (listeners.empty())
			m_Listeners.erase(iter);
	}

	// 특정 owner의 해당 이벤트 구독을 모두 해제
	template<typename TEvent>
	void UnsubscribeAll(CHandle owner)
	{
		auto iter = m_Listeners.find(typeid(TEvent));
		if (iter == m_Listeners.end())
			return;

		auto& listeners = iter->second;

		std::erase_if(
			listeners,
			[owner](const LISTENER& listener)
			{
				return listener.owner == owner;
			}
		);

		if (listeners.empty())
			m_Listeners.erase(iter);
	}

	// 특정 owner의 모든 이벤트 구독을 해제
	void UnsubscribeAll(CHandle owner);

	template<typename TEvent>
	void Publish(TEvent&& event)
	{
		using EVENT_TYPE = std::decay_t<TEvent>;

		QUEUED_EVENT queuedEvent{
			typeid(EVENT_TYPE),
			std::make_shared<EVENT_TYPE>(std::forward<TEvent>(event))
		};

		PushEvent(std::move(queuedEvent));
	}

	template<typename TEvent>
	void Publish()
	{
		static_assert(std::is_default_constructible_v<TEvent>,
			"Payload-free events must be default constructible.");
		Publish(TEvent{});
	}

	void FrameEnd();

private:
	void PushEvent(QUEUED_EVENT event);
	void RemoveDeadListeners(const std::vector<CHandle>& deadOwners);
	void Flush();

private:
	std::unordered_map<std::type_index, std::vector<LISTENER>> m_Listeners; // 이벤트 타입 별 구독자들
	std::queue<QUEUED_EVENT> m_CurQueue; // 이번 프레임에 소비하는 Queue
	std::queue<QUEUED_EVENT> m_NextQueue; // Flush하는동안 무한 구독 방지용
	EVENT_LISTENER_ID m_NextListenerId = 1;
	_bool m_bIsFlushing = false;

public:
	static UPtr<CEventManager> Create();
	void Free() override;
};

NS_END
