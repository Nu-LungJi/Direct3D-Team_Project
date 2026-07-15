#include "pch.h"
#include "EditorCommandManager.h"

#include "GameInstance.h"

NS_USING(Client)

void CEditorCommandManager::Submit(std::unique_ptr<IEditorCommand> command)
{
	if (command)
		m_RequestQueue.push_back({ REQUEST_TYPE::EXECUTE, std::move(command) });
}

void CEditorCommandManager::RequestUndo()
{
	m_RequestQueue.push_back({ REQUEST_TYPE::UNDO, nullptr });
}

void CEditorCommandManager::RequestRedo()
{
	m_RequestQueue.push_back({ REQUEST_TYPE::REDO, nullptr });
}

void CEditorCommandManager::ProcessOne()
{
	// Pending-destroy objects are removed at FrameEnd, so rebuild on the next GUI frame.
	if (m_bRebuildChunksNextFrame)
	{
		E::CGameInstance::Get().RebuildMapChunks();
		m_bRebuildChunksNextFrame = false;
	}

	if (m_RequestQueue.empty())
		return;

	auto request = std::move(m_RequestQueue.front());
	m_RequestQueue.pop_front();

	_bool succeeded = false;
	switch (request.type)
	{
	case REQUEST_TYPE::EXECUTE:
		succeeded = ExecuteNow(std::move(request.command));
		break;
	case REQUEST_TYPE::UNDO:
		succeeded = UndoNow();
		break;
	case REQUEST_TYPE::REDO:
		succeeded = RedoNow();
		break;
	}

	if (succeeded)
		m_bRebuildChunksNextFrame = true;
}

void CEditorCommandManager::Clear()
{
	m_RequestQueue.clear();
	m_UndoStack.clear();
	m_RedoStack.clear();
	m_bRebuildChunksNextFrame = false;
}

_bool CEditorCommandManager::ExecuteNow(std::unique_ptr<IEditorCommand> command)
{
	if (!command || !command->Execute())
		return false;

	m_UndoStack.push_back(std::move(command));
	if (m_UndoStack.size() > MAX_HISTORY)
		m_UndoStack.erase(m_UndoStack.begin());
	m_RedoStack.clear();
	return true;
}

_bool CEditorCommandManager::UndoNow()
{
	if (m_UndoStack.empty())
		return false;

	auto command = std::move(m_UndoStack.back());
	m_UndoStack.pop_back();
	if (!command->Undo())
	{
		m_UndoStack.push_back(std::move(command));
		return false;
	}

	m_RedoStack.push_back(std::move(command));
	return true;
}

_bool CEditorCommandManager::RedoNow()
{
	if (m_RedoStack.empty())
		return false;

	auto command = std::move(m_RedoStack.back());
	m_RedoStack.pop_back();
	if (!command->Execute())
	{
		m_RedoStack.push_back(std::move(command));
		return false;
	}

	m_UndoStack.push_back(std::move(command));
	return true;
}

E::UPtr<CEditorCommandManager> CEditorCommandManager::Create()
{
	return E::ToUPtr(new CEditorCommandManager{});
}
