#include "pch.h"
#include "EditorSelection.h"

#include "GameInstance.h"

NS_USING(Client)

CEditorSelection::CEditorSelection(E::CHandle* primaryHandle)
	: m_pPrimaryHandle{ primaryHandle }
{
	SyncFromPrimary();
}

void CEditorSelection::SyncFromPrimary()
{
	if (m_pPrimaryHandle == nullptr || *m_pPrimaryHandle == m_LastPrimaryHandle)
		return;

	if (E::CGameInstance::Get().GetGameObjectByHandle(*m_pPrimaryHandle) != nullptr)
		SelectSingle(*m_pPrimaryHandle);
	else
	{
		const auto previousPrimary = std::find(
			m_SelectedHandles.begin(), m_SelectedHandles.end(), m_LastPrimaryHandle);
		if (previousPrimary != m_SelectedHandles.end())
			m_SelectedHandles.erase(previousPrimary);

		if (m_SelectedHandles.empty())
			Clear();
		else
			SetPrimary(m_SelectedHandles.back());
	}
}

void CEditorSelection::PruneInvalid()
{
	std::erase_if(m_SelectedHandles, [](const E::CHandle& handle)
	{
		return E::CGameInstance::Get().GetGameObjectByHandle(handle) == nullptr;
	});

	if (m_SelectedHandles.empty())
	{
		SetPrimary(E::CHandle{});
		return;
	}

	if (m_pPrimaryHandle == nullptr)
		return;

	if (!IsSelected(*m_pPrimaryHandle))
		SetPrimary(m_SelectedHandles.back());
	else
		m_LastPrimaryHandle = *m_pPrimaryHandle;
}

void CEditorSelection::SelectSingle(const E::CHandle& handle)
{
	m_SelectedHandles.clear();
	if (E::CGameInstance::Get().GetGameObjectByHandle(handle) != nullptr)
	{
		m_SelectedHandles.push_back(handle);
		SetPrimary(handle);
	}
	else
	{
		SetPrimary(E::CHandle{});
	}
}

void CEditorSelection::Toggle(const E::CHandle& handle)
{
	const auto iter = std::find(m_SelectedHandles.begin(), m_SelectedHandles.end(), handle);
	if (iter == m_SelectedHandles.end())
	{
		if (E::CGameInstance::Get().GetGameObjectByHandle(handle) == nullptr)
			return;

		m_SelectedHandles.push_back(handle);
		SetPrimary(handle);
		return;
	}

	m_SelectedHandles.erase(iter);
	if (m_SelectedHandles.empty())
		SetPrimary(E::CHandle{});
	else if (*m_pPrimaryHandle == handle)
		SetPrimary(m_SelectedHandles.back());
}

void CEditorSelection::SelectMany(const std::vector<E::CHandle>& handles)
{
	m_SelectedHandles.clear();
	for (const E::CHandle& handle : handles)
	{
		if (E::CGameInstance::Get().GetGameObjectByHandle(handle) != nullptr &&
			!IsSelected(handle))
		{
			m_SelectedHandles.push_back(handle);
		}
	}

	if (m_SelectedHandles.empty())
		SetPrimary(E::CHandle{});
	else
		SetPrimary(m_SelectedHandles.back());
}

void CEditorSelection::SelectRange(const std::vector<E::CHandle>& handles,
	size_t firstIndex, size_t lastIndex, _bool additive)
{
	if (handles.empty() || firstIndex >= handles.size() || lastIndex >= handles.size())
		return;

	const size_t clickedIndex = lastIndex;
	if (firstIndex > lastIndex)
		std::swap(firstIndex, lastIndex);

	if (!additive)
		m_SelectedHandles.clear();

	for (size_t index = firstIndex; index <= lastIndex; ++index)
	{
		const E::CHandle& handle = handles[index];
		if (E::CGameInstance::Get().GetGameObjectByHandle(handle) == nullptr || IsSelected(handle))
			continue;
		m_SelectedHandles.push_back(handle);
	}

	// The clicked end of the range becomes the primary object for the inspector
	// and multi-object gizmo pivot, while the hierarchy anchor remains unchanged.
	const E::CHandle& clickedHandle = handles[clickedIndex];
	if (IsSelected(clickedHandle))
		SetPrimary(clickedHandle);
	else if (m_SelectedHandles.empty())
		SetPrimary(E::CHandle{});
}

void CEditorSelection::Clear()
{
	m_SelectedHandles.clear();
	SetPrimary(E::CHandle{});
}

_bool CEditorSelection::IsSelected(const E::CHandle& handle) const
{
	return std::find(m_SelectedHandles.begin(), m_SelectedHandles.end(), handle) !=
		m_SelectedHandles.end();
}

void CEditorSelection::SetPrimary(const E::CHandle& handle)
{
	if (m_pPrimaryHandle)
		*m_pPrimaryHandle = handle;
	m_LastPrimaryHandle = handle;
}
