#pragma once

#include "IEditorCommand.h"
#include "MapMeshCommandCommon.h"

NS_BEGIN(Client)

class CScatterObjectsCommand final : public IEditorCommand
{
public:
	CScatterObjectsCommand(std::vector<MAPMESH_OBJECT_SNAPSHOT> snapshots,
		std::vector<E::CHandle> handles);
	_bool Execute() override;
	_bool Undo() override;

private:
	std::vector<MAPMESH_OBJECT_SNAPSHOT> m_Snapshots{};
	std::vector<E::CHandle> m_Handles{};
};

NS_END
