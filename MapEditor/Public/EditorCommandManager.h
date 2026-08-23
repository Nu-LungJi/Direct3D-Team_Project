#pragma once
#include "Engine_Defines.h"
#include "IEditorCommand.h"

NS_BEGIN(Client)

class CEditorCommandManager final : public E::CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CEditorCommandManager, E::CEngineBase)

public:
	void Submit(std::unique_ptr<IEditorCommand> command);
	void SubmitBatch(std::vector<std::unique_ptr<IEditorCommand>> commands);
	void RecordExecuted(std::unique_ptr<IEditorCommand> command);
	void RequestUndo();
	void RequestRedo();
	void ProcessOne();
	void Clear();

	_bool CanUndo() const { return !m_UndoStack.empty(); }
	_bool CanRedo() const { return !m_RedoStack.empty(); }

	static E::UPtr<CEditorCommandManager> Create();

private:
	enum class REQUEST_TYPE : uint8_t
	{
		EXECUTE,
		EXECUTE_BATCH,
		UNDO,
		REDO
	};

	struct COMMAND_REQUEST
	{
		REQUEST_TYPE type = REQUEST_TYPE::EXECUTE;
		std::unique_ptr<IEditorCommand> command{};
		std::vector<std::unique_ptr<IEditorCommand>> batchCommands{};
		size_t nextBatchCommand{};
		_bool batchChanged{};
	};

	_bool ExecuteNow(std::unique_ptr<IEditorCommand> command);
	_bool UndoNow();
	_bool RedoNow();

private:
	static constexpr size_t MAX_HISTORY = 100;
	static constexpr size_t MAX_BATCH_COMMANDS_PER_FRAME = 1000;
	std::deque<COMMAND_REQUEST> m_RequestQueue{};
	std::vector<std::unique_ptr<IEditorCommand>> m_UndoStack{};
	std::vector<std::unique_ptr<IEditorCommand>> m_RedoStack{};
	_bool m_bRebuildChunksNextFrame = false;
};

NS_END
