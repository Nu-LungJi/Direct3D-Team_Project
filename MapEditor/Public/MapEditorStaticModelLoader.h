#pragma once

#include "MapStaticModelLoader.h"

NS_BEGIN(Client)

using MAPEDITOR_MODEL_LOAD_RESULT = E::MAP_STATIC_MODEL_LOAD_RESULT;

inline std::string MakeMapEditorStaticModelTag(
	const std::filesystem::path& rootPath, const std::filesystem::path& binPath)
{
	return E::MakeMapStaticModelTag(rootPath, binPath);
}

inline bool LoadMapEditorStaticModelFile(const std::filesystem::path& binPath,
	const std::filesystem::path& staticRoot, std::string* outTag = nullptr,
	const std::string& requestedTag = {})
{
	return E::LoadMapStaticModelFile(binPath, staticRoot,
		E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL, outTag, requestedTag);
}

inline MAPEDITOR_MODEL_LOAD_RESULT LoadMapEditorStaticModelFolder(
	const std::filesystem::path& selectedFolder)
{
	MAPEDITOR_MODEL_LOAD_RESULT result{};
	const std::filesystem::path root = E::PATH_MAPEDITOR_STATIC_MODEL_DIR;
	std::error_code ec{};
	if (!std::filesystem::is_directory(selectedFolder, ec))
	{
		result.failed.push_back(selectedFolder.string());
		return result;
	}
	for (const auto& entry : std::filesystem::recursive_directory_iterator(
		selectedFolder, std::filesystem::directory_options::skip_permission_denied, ec))
	{
		if (ec) break;
		if (!entry.is_regular_file(ec) ||
			_stricmp(entry.path().extension().string().c_str(), ".bin") != 0) continue;
		++result.requested;
		const std::string tag = MakeMapEditorStaticModelTag(root, entry.path());
		if (E::CGameInstance::Get().GetResourceFirst<E::CResStaticModel>(
			E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL, tag))
		{
			++result.cached;
			continue;
		}
		if (LoadMapEditorStaticModelFile(entry.path(), root)) ++result.loaded;
		else result.failed.push_back(entry.path().string());
	}
	return result;
}

NS_END
