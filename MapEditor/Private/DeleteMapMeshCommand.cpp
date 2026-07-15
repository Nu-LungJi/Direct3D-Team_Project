#include "pch.h"
#include "DeleteMapMeshCommand.h"

NS_USING(Client)

CDeleteMapMeshCommand::CDeleteMapMeshCommand(
	E::CHandle handle, MAPMESH_OBJECT_SNAPSHOT snapshot, E::CHandle* selectedHandle)
	: m_CurrentHandle{ handle }
	, m_Snapshot{ std::move(snapshot) }
	, m_pSelectedHandle{ selectedHandle }
{
}

_bool CDeleteMapMeshCommand::Execute()
{
	if (!m_CurrentHandle || !DestroyMapMeshObject(*m_CurrentHandle))
		return false;

	if (m_pSelectedHandle && *m_pSelectedHandle == *m_CurrentHandle)
		*m_pSelectedHandle = E::CHandle{};
	return true;
}

_bool CDeleteMapMeshCommand::Undo()
{
	const auto handle = SpawnMapMeshObject(m_Snapshot);
	if (!handle)
		return false;

	m_CurrentHandle = *handle;
	if (m_pSelectedHandle)
		*m_pSelectedHandle = *handle;
	return true;
}
