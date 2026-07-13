#include "pch.h"
#include "CreateMapMeshCommand.h"

NS_USING(Client)

CCreateMapMeshCommand::CCreateMapMeshCommand(
	MAPMESH_OBJECT_SNAPSHOT snapshot, E::CHandle* selectedHandle)
	: m_Snapshot{ std::move(snapshot) }
	, m_pSelectedHandle{ selectedHandle }
{
}

_bool CCreateMapMeshCommand::Execute()
{
	const auto handle = SpawnMapMeshObject(m_Snapshot);
	if (!handle)
		return false;

	m_CurrentHandle = *handle;
	if (m_pSelectedHandle)
		*m_pSelectedHandle = *handle;
	return true;
}

_bool CCreateMapMeshCommand::Undo()
{
	if (!m_CurrentHandle || !DestroyMapMeshObject(*m_CurrentHandle))
		return false;

	if (m_pSelectedHandle && *m_pSelectedHandle == *m_CurrentHandle)
		*m_pSelectedHandle = E::CHandle{};
	return true;
}
