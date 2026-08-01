#pragma once

#include "GameInstance.h"
#include "Resources.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_set>

namespace Engine
{
	struct MAP_STATIC_MODEL_LOAD_RESULT
	{
		size_t requested{};
		size_t loaded{};
		size_t cached{};
		std::vector<std::string> missing{};
		std::vector<std::string> failed{};
		std::unordered_map<std::string, std::filesystem::path> modelPaths{};

		bool Succeeded() const { return missing.empty() && failed.empty(); }
	};

	inline std::string MakeMapStaticModelTag(const std::filesystem::path& rootPath, const std::filesystem::path& binPath)
	{
		std::filesystem::path relativePath = binPath.lexically_relative(rootPath);
		if (relativePath.empty() || (!relativePath.empty() && *relativePath.begin() == ".."))
			relativePath = binPath.filename();
		relativePath.replace_extension();
		std::string tag = relativePath.string();
		for (char& ch : tag)
			if (!std::isalnum(static_cast<unsigned char>(ch))) ch = '_';
		return tag;
	}

	inline bool LoadMapStaticModelFile(const std::filesystem::path& binPath,const std::filesystem::path& staticRoot, const std::string& resourceGroup,
		std::string* outTag = nullptr, const std::string& requestedTag = {})
	{
		const std::string tag = requestedTag.empty() ? MakeMapStaticModelTag(staticRoot, binPath) : requestedTag;
		if (outTag) *outTag = tag;
		if (auto cached = CGameInstance::Get().GetResourceFirst<CResStaticModel>(resourceGroup, tag))
		{
			if (cached->GetState() == CResource::STATE::LOADED)
				return true;
			if (cached->GetState() == CResource::STATE::LOADFAIL || cached->GetState() == CResource::STATE::UNLOAD)
				CGameInstance::Get().DelResource(resourceGroup, tag);
			else
				return false;
		}

		auto resource = CGameInstance::Get().AddResourceT<CResStaticModel>(resourceGroup, tag, CResStaticModel::Create(binPath.string()));
		if (!resource) return false;
		CResStaticModel::DESC desc{};
		desc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f); // * XMMatrixRotationAxis({ 1.f, 0.f, 0.f }, XMConvertToRadians(90.f));
		if (SUCCEEDED(resource->Load(desc)))
			return true;
		CGameInstance::Get().DelResource(resourceGroup, tag);
		return false;
	}

	inline MAP_STATIC_MODEL_LOAD_RESULT IndexStaticModelsRequiredByMap(const std::filesystem::path& mapDirectory, const std::filesystem::path& staticRoot,
		const std::string& resourceGroup)
	{
		MAP_STATIC_MODEL_LOAD_RESULT result{};
		std::unordered_set<std::string> requiredTags{};
		auto CollectObjects = [&](const nlohmann::json& objects)
			{
				if (!objects.is_array()) return;
				for (const auto& object : objects)
				{
					if (object.value("type", std::string{}) != "MapMeshObject") continue;
					if (object.value("modelGroup", std::string{}) != resourceGroup) continue;
					const std::string tag = object.value("model", std::string{});
					if (!tag.empty()) requiredTags.insert(tag);
				}
			};

		try
		{
			const auto ReadJson = [](const std::filesystem::path& filePath)
				{
					std::ifstream stream(filePath);
					nlohmann::json value{};
					if (stream.is_open()) stream >> value;
					return value;
				};
			const auto mapJson = ReadJson(mapDirectory / "map.json");
			if (mapJson.contains("objects")) CollectObjects(mapJson["objects"]);
			if (mapJson.contains("chunks") && mapJson["chunks"].is_array())
			{
				for (const auto& chunk : mapJson["chunks"])
				{
					const auto chunkJson = ReadJson(mapDirectory /
						chunk.value("file", std::string{}));
					if (chunkJson.contains("objects")) CollectObjects(chunkJson["objects"]);
				}
			}
			const auto legacyJson = ReadJson(mapDirectory / "TestMap.json");
			if (legacyJson.contains("objects")) CollectObjects(legacyJson["objects"]);
		}
		catch (const std::exception&)
		{
			result.failed.push_back(mapDirectory.string());
			return result;
		}

		std::unordered_map<std::string, std::filesystem::path> byTag{};
		std::unordered_map<std::string, std::filesystem::path> byStem{};
		std::error_code ec{};
		for (const auto& entry : std::filesystem::recursive_directory_iterator(
			staticRoot, std::filesystem::directory_options::skip_permission_denied, ec))
		{
			if (ec) break;
			if (!entry.is_regular_file(ec) ||
				_stricmp(entry.path().extension().string().c_str(), ".bin") != 0) continue;
			byTag.emplace(MakeMapStaticModelTag(staticRoot, entry.path()), entry.path());
			byStem.emplace(entry.path().stem().string(), entry.path());
		}

		result.requested = requiredTags.size();
		for (const auto& tag : requiredTags)
		{
			auto found = byTag.find(tag);
			if (found == byTag.end())
			{
				const auto stemFound = byStem.find(tag);
				if (stemFound == byStem.end())
				{
					result.missing.push_back(tag);
					continue;
				}
				found = byTag.emplace(tag, stemFound->second).first;
			}
			result.modelPaths.emplace(tag, found->second);
			++result.loaded;
		}
		return result;
	}
}
