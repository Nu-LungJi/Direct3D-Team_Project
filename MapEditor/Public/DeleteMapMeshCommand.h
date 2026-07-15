#pragma once
#include "IEditorCommand.h"
#include "MapMeshCommandCommon.h"

NS_BEGIN(Client)

class CDeleteMapMeshCommand final : public IEditorCommand
{
public:
	CDeleteMapMeshCommand(E::CHandle handle, MAPMESH_OBJECT_SNAPSHOT snapshot,
		E::CHandle* selectedHandle);
	_bool Execute() override;
	_bool Undo() override;

private:
	std::optional<E::CHandle> m_CurrentHandle{};
	MAPMESH_OBJECT_SNAPSHOT m_Snapshot{};
	E::CHandle* m_pSelectedHandle = nullptr;
};

NS_END
