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
