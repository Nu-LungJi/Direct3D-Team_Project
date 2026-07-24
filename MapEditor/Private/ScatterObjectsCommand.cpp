#include "pch.h"
#include "ScatterObjectsCommand.h"

NS_USING(Client)

CScatterObjectsCommand::CScatterObjectsCommand(
	std::vector<MAPMESH_OBJECT_SNAPSHOT> snapshots, std::vector<E::CHandle> handles)
	: m_Snapshots{ std::move(snapshots) }, m_Handles{ std::move(handles) } {}

_bool CScatterObjectsCommand::Execute()
{
	m_Handles.clear();
	for (const auto& snapshot : m_Snapshots)
	{
		auto handle = SpawnMapMeshObject(snapshot);
		if (!handle) return false;
		m_Handles.push_back(*handle);
	}
	return !m_Handles.empty();
}

_bool CScatterObjectsCommand::Undo()
{
	bool success = true;
	for (const auto& handle : m_Handles) success &= DestroyMapMeshObject(handle);
	m_Handles.clear();
	return success;
}
