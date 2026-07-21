#include "pch.h"
#include "EventManager.h"

CEventManager::CEventManager()
{
}

CEventManager::~CEventManager()
{
}

HRESULT CEventManager::Initialize()
{
	return S_OK;
}

void CEventManager::PushEvent(QUEUED_EVENT event)
{
	if (m_bIsFlushing)
	{
		m_NextQueue.push(std::move(event));
	}
	else
	{
		m_CurQueue.push(std::move(event));
	}
}

void CEventManager::UnsubscribeAll(CHandle owner)
{
	for (auto mapIter = m_Listeners.begin(); mapIter != m_Listeners.end();)
	{
		auto& listeners = mapIter->second;
		std::erase_if(listeners, [owner](const LISTENER& listener)
			{
				return listener.owner == owner;
			});

		if (listeners.empty())
			mapIter = m_Listeners.erase(mapIter);
		else
			++mapIter;
	}
}

void CEventManager::Flush()
{
	m_bIsFlushing = true;

	std::vector<CHandle> deadOwners;

	while (!m_CurQueue.empty())
	{
		QUEUED_EVENT event = std::move(m_CurQueue.front());
		m_CurQueue.pop();

		auto iter = m_Listeners.find(event.type);
		if (iter == m_Listeners.end())
			continue;

		const auto listeners = iter->second; // 이벤트 콜백 중 같은 이벤트를 구독할 수 있어서 auto& 안하고 복사
		for (const auto& listener : listeners)
		{
			auto* owner = CGameInstance::Get().GetGameObjectByHandle(listener.owner);
			if (owner && !owner->GetPendingDestroy())
			{
				if (listener.callback)
					listener.callback(event.payload.get());
			}
			else
			{
				deadOwners.push_back(listener.owner);
			}
		}
	}

	RemoveDeadListeners(deadOwners);
	m_CurQueue.swap(m_NextQueue);
	m_bIsFlushing = false;
}

void CEventManager::RemoveDeadListeners(const std::vector<CHandle>& deadOwners)
{
	if (deadOwners.empty())
		return;

	for (auto mapIter = m_Listeners.begin(); mapIter != m_Listeners.end();)
	{
		auto& listeners = mapIter->second;

		std::erase_if(listeners, [&deadOwners](const LISTENER& listener)
			{
				return std::ranges::find(deadOwners, listener.owner) != deadOwners.end();
			});

		if (listeners.empty())
			mapIter = m_Listeners.erase(mapIter);
		else
			++mapIter;
	}
}

void CEventManager::FrameEnd()
{
	Flush();
}

UPtr<CEventManager> CEventManager::Create()
{
	auto pInstance = ToUPtr(new CEventManager{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

void CEventManager::Free()
{
	CEngineBase::Free();
}
