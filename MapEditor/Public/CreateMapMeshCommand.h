#pragma once
#include "IEditorCommand.h"
#include "MapMeshCommandCommon.h"

NS_BEGIN(Client)

class CCreateMapMeshCommand final : public IEditorCommand
{
public:
	CCreateMapMeshCommand(MAPMESH_OBJECT_SNAPSHOT snapshot, E::CHandle* selectedHandle);
	_bool Execute() override;
	_bool Undo() override;

private:
	MAPMESH_OBJECT_SNAPSHOT m_Snapshot{};
	std::optional<E::CHandle> m_CurrentHandle{};
	E::CHandle* m_pSelectedHandle = nullptr;
};

NS_END
