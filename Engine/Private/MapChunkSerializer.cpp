#include "pch.h"
#include "MapChunkSerializer.h"
#include <fstream>

NS_USING(Engine)

std::string CMapChunkSerializer::MakeChunkFileName(const MAPCHUNK_COORD& coord) const
{
	return std::to_string(coord.x) + "_"
		+ std::to_string(coord.y) + "_"
		+ std::to_string(coord.z) + ".json";
}

HRESULT CMapChunkSerializer::SaveMapFile(const std::filesystem::path& filePath, const MAP_FILE_DATA& mapData) const
{
	try
	{
		std::error_code error;
		std::filesystem::create_directories(filePath.parent_path(), error);
		if (error)
			return E_FAIL;

		nlohmann::ordered_json rootJson{};
		rootJson["version"] = mapData.version;
		rootJson["chunkSize"] =
		{
			mapData.chunkSize.x,
			mapData.chunkSize.y,
			mapData.chunkSize.z
		};
		rootJson["chunks"] = nlohmann::ordered_json::array();
		rootJson["requiredModels"] = nlohmann::ordered_json::array();
		rootJson["decals"] = nlohmann::ordered_json::array();

		for (const auto& chunk : mapData.chunks)
		{
			rootJson["chunks"].push_back(
			{
				{ "coord", WriteCoord(chunk.coord) },
				{ "file", chunk.filePath },
				{ "objectCount", chunk.objectCount }
			});
		}

		for (const auto& model : mapData.requiredModels)
		{
			rootJson["requiredModels"].push_back(
			{
				{ "modelGroup", model.group },
				{ "model", model.tag }
			});
		}

		for (const auto& decal : mapData.decals)
			rootJson["decals"].push_back(WriteDecal(decal));

		std::ofstream outFile(filePath);
		if (!outFile.is_open())
			return E_FAIL;

		outFile << rootJson.dump(4);
		return outFile.good() ? S_OK : E_FAIL;
	}
	catch (const std::exception&)
	{
		return E_FAIL;
	}
}

HRESULT CMapChunkSerializer::LoadMapFile(const std::filesystem::path& filePath, MAP_FILE_DATA& outMapData) const
{
	try
	{
		std::ifstream inFile(filePath);
		if (!inFile.is_open())
			return E_FAIL;

		nlohmann::ordered_json rootJson;
		inFile >> rootJson;

		MAP_FILE_DATA loadedData{};
		loadedData.version = rootJson.value("version", loadedData.version);
		if (rootJson.contains("chunkSize"))
		{
			const auto& size = rootJson["chunkSize"];
			loadedData.chunkSize = { size[0], size[1], size[2] };
		}

		if (rootJson.contains("chunks") && rootJson["chunks"].is_array())
		{
			for (const auto& chunkJson : rootJson["chunks"])
			{
				MAP_CHUNK_METADATA chunk{};
				chunk.coord = ReadCoord(chunkJson["coord"]);
				chunk.filePath = chunkJson.value(
					"file",
					(std::filesystem::path("chunks") /
						MakeChunkFileName(chunk.coord)).generic_string());
				chunk.objectCount = chunkJson.value("objectCount", size_t{});
				loadedData.chunks.push_back(std::move(chunk));
			}
		}

		if (rootJson.contains("requiredModels") && rootJson["requiredModels"].is_array())
		{
			for (const auto& modelJson : rootJson["requiredModels"])
			{
				loadedData.requiredModels.push_back(
				{
					modelJson.value("modelGroup", std::string{}),
					modelJson.value("model", std::string{})
				});
			}
		}

		if (rootJson.contains("decals") && rootJson["decals"].is_array())
		{
			for (const auto& decalJson : rootJson["decals"])
			{
				auto decal = ReadDecal(decalJson);
				if (!decal)
					return E_FAIL;
				loadedData.decals.push_back(std::move(*decal));
			}
		}

		outMapData = std::move(loadedData);
		return S_OK;
	}
	catch (const std::exception&)
	{
		return E_FAIL;
	}
}

HRESULT CMapChunkSerializer::SaveChunkFile(const std::filesystem::path& filePath, const MAP_CHUNK_FILE_DATA& chunkData) const
{
	try
	{
		std::error_code error;
		std::filesystem::create_directories(filePath.parent_path(), error);
		if (error)
			return E_FAIL;

		nlohmann::ordered_json chunkJson{};
		chunkJson["version"] = chunkData.version;
		chunkJson["coord"] = WriteCoord(chunkData.coord);
		chunkJson["bounds"] =
		{
			{ "center", {
				chunkData.bounds.Center.x,
				chunkData.bounds.Center.y,
				chunkData.bounds.Center.z } },
			{ "extents", {
				chunkData.bounds.Extents.x,
				chunkData.bounds.Extents.y,
				chunkData.bounds.Extents.z } }
		};
		chunkJson["objects"] = nlohmann::ordered_json::array();
		for (const auto& object : chunkData.objects)
			chunkJson["objects"].push_back(WriteMapMeshObject(object, chunkData.coord));

		std::ofstream outFile(filePath);
		if (!outFile.is_open())
			return E_FAIL;

		outFile << chunkJson.dump(4);
		return outFile.good() ? S_OK : E_FAIL;
	}
	catch (const std::exception&)
	{
		return E_FAIL;
	}
}

HRESULT CMapChunkSerializer::LoadChunkFile(const std::filesystem::path& filePath, MAP_CHUNK_FILE_DATA& outChunkData) const
{
	try
	{
		std::ifstream inFile(filePath);
		if (!inFile.is_open())
			return E_FAIL;

		nlohmann::ordered_json chunkJson;
		inFile >> chunkJson;

		MAP_CHUNK_FILE_DATA loadedData{};
		loadedData.version = chunkJson.value("version", loadedData.version);
		if (chunkJson.contains("coord"))
			loadedData.coord = ReadCoord(chunkJson["coord"]);

		if (chunkJson.contains("bounds") && chunkJson["bounds"].is_object())
		{
			loadedData.bounds.Center = ReadFloat3(chunkJson["bounds"], "center", {});
			loadedData.bounds.Extents = ReadFloat3(chunkJson["bounds"], "extents", {});
		}

		if (chunkJson.contains("objects") && chunkJson["objects"].is_array())
		{
			loadedData.objects.reserve(chunkJson["objects"].size());
			for (const auto& objectJson : chunkJson["objects"])
				if (auto object = ReadMapMeshObject(objectJson))
					loadedData.objects.push_back(std::move(*object));
		}

		outChunkData = std::move(loadedData);
		return S_OK;
	}
	catch (const std::exception&)
	{
		return E_FAIL;
	}
}

HRESULT CMapChunkSerializer::LoadLegacyMapFile(const std::filesystem::path& filePath, std::vector<MAP_MESH_OBJECT_FILE_DATA>& outObjects) const
{
	try
	{
		std::ifstream inFile(filePath);
		if (!inFile.is_open())
			return E_FAIL;

		nlohmann::ordered_json rootJson;
		inFile >> rootJson;

		std::vector<MAP_MESH_OBJECT_FILE_DATA> loadedObjects;
		if (rootJson.contains("objects") && rootJson["objects"].is_array())
		{
			loadedObjects.reserve(rootJson["objects"].size());
			for (const auto& objectJson : rootJson["objects"])
			{
				if (auto object = ReadMapMeshObject(objectJson))
					loadedObjects.push_back(std::move(*object));
			}
		}

		outObjects = std::move(loadedObjects);
		return S_OK;
	}
	catch (const std::exception&)
	{
		return E_FAIL;
	}
}

nlohmann::ordered_json CMapChunkSerializer::WriteCoord(const MAPCHUNK_COORD& coord) const
{
	return
	{
		{ "x", coord.x },
		{ "y", coord.y },
		{ "z", coord.z }
	};
}

MAPCHUNK_COORD CMapChunkSerializer::ReadCoord(const nlohmann::ordered_json& json) const
{
	return
	{
		json["x"].get<int64_t>(),
		json["y"].get<int64_t>(),
		json["z"].get<int64_t>()
	};
}

nlohmann::ordered_json CMapChunkSerializer::WriteWind(const WIND_DESC& windDesc) const
{
	return
	{
		{ "type", static_cast<uint32_t>(windDesc.type) },
		{ "strength", windDesc.strength },
		{ "speed", windDesc.speed },
		{ "frequency", windDesc.frequency },
		{ "bendExponent", windDesc.bendExponent },
		{ "heightStart", windDesc.heightStart },
		{ "heightEnd", windDesc.heightEnd }
	};
}

WIND_DESC CMapChunkSerializer::ReadWind(const nlohmann::ordered_json& json) const
{
	WIND_DESC windDesc{};
	if (!json.contains("wind") || !json["wind"].is_object())
		return windDesc;

	const auto& windJson = json["wind"];
	const uint32_t windType = windJson.value("type", static_cast<uint32_t>(EWindType::None));

	if (windType <= static_cast<uint32_t>(EWindType::Tree))
		windDesc.type = static_cast<EWindType>(windType);

	windDesc.strength = windJson.value("strength", windDesc.strength);
	windDesc.speed = windJson.value("speed", windDesc.speed);
	windDesc.frequency = windJson.value("frequency", windDesc.frequency);
	windDesc.bendExponent = windJson.value("bendExponent", windDesc.bendExponent);
	windDesc.heightStart = windJson.value("heightStart", windDesc.heightStart);
	windDesc.heightEnd = windJson.value("heightEnd", windDesc.heightEnd);
	return windDesc;
}

nlohmann::ordered_json CMapChunkSerializer::WriteMapMeshObject(const MAP_MESH_OBJECT_FILE_DATA& object, const MAPCHUNK_COORD& coord) const
{
	return
	{
		{ "type", "MapMeshObject" },
		{ "objectTag", object.objectTag },
		{ "protoGroup", object.protoGroup },
		{ "prototype", object.prototype },
		{ "modelGroup", object.modelGroup },
		{ "model", object.model },
		{ "layer", object.layer },
		{ "position", { object.position.x, object.position.y, object.position.z } },
		{ "rotation", { object.rotation.x, object.rotation.y, object.rotation.z, object.rotation.w } },
		{ "scale", { object.scale.x, object.scale.y, object.scale.z } },
		{ "wind", WriteWind(object.windDesc) },
		{ "chunk", WriteCoord(coord) }
	};
}

std::optional<MAP_MESH_OBJECT_FILE_DATA> CMapChunkSerializer::ReadMapMeshObject(const nlohmann::ordered_json& json) const
{
	if (!json.contains("type") || json["type"] != "MapMeshObject")
		return std::nullopt;

	MAP_MESH_OBJECT_FILE_DATA object{};
	object.objectTag = json["objectTag"];
	object.protoGroup = json.value("protoGroup", "PERMANENT");
	object.prototype = json.value("prototype", "Prototype_GameObject_MapMeshObject");
	object.modelGroup = json["modelGroup"];
	object.model = json["model"];
	object.layer = json["layer"];
	object.position = ReadFloat3(json, "position", {});
	object.rotation = ReadFloat4(json, "rotation", { 0.f, 0.f, 0.f, 1.f });
	object.scale = ReadFloat3(json, "scale", { 1.f, 1.f, 1.f });
	object.windDesc = ReadWind(json);
	return object;
}

nlohmann::ordered_json CMapChunkSerializer::WriteDecal(const MAP_DECAL_FILE_DATA& decal) const
{
	nlohmann::ordered_json result
	{
		{ "type", "DecalVolume" },
		{ "objectTag", decal.objectTag },
		{ "protoGroup", decal.protoGroup },
		{ "prototype", decal.prototype },
		{ "layer", decal.layer },
		{ "position", { decal.position.x, decal.position.y, decal.position.z } },
		{ "rotation", { decal.rotation.x, decal.rotation.y, decal.rotation.z, decal.rotation.w } },
		{ "scale", { decal.scale.x, decal.scale.y, decal.scale.z } },
		{ "materialPath", decal.materialPath },
		{ "opacity", decal.opacity },
		{ "normalThreshold", decal.normalThreshold },
		{ "edgeSoftness", decal.edgeSoftness },
		{ "textureGroup", decal.textureGroup },
		{ "textureTag", decal.textureTag },
		{ "texturePath", decal.texturePath }
	};

	auto parametersJson = nlohmann::ordered_json::object();
	for (const auto& parameter : decal.materialParameters)
	{
		if (parameter.values.size() == 1)
			parametersJson[parameter.name] = parameter.values.front();
		else
			parametersJson[parameter.name] = parameter.values;
	}
	result["materialParameters"] = std::move(parametersJson);

	auto texturesJson = nlohmann::ordered_json::array();
	for (const auto& texture : decal.textureOverrides)
	{
		texturesJson.push_back(
		{
			{ "slot", texture.slot },
			{ "group", texture.group },
			{ "tag", texture.tag },
			{ "path", texture.path }
		});
	}
	result["textureOverrides"] = std::move(texturesJson);
	return result;
}

std::optional<MAP_DECAL_FILE_DATA> CMapChunkSerializer::ReadDecal(const nlohmann::ordered_json& json) const
{
	if (!json.is_object() || json.value("type", std::string{}) != "DecalVolume")
		return std::nullopt;

	MAP_DECAL_FILE_DATA decal{};
	decal.objectTag = json.value("objectTag", decal.objectTag);
	decal.protoGroup = json.value("protoGroup", decal.protoGroup);
	decal.prototype = json.value("prototype", decal.prototype);
	decal.layer = json.value("layer", std::string{});
	decal.materialPath = json.value("materialPath", std::string{});
	decal.textureGroup = json.value("textureGroup", std::string{});
	decal.textureTag = json.value("textureTag", std::string{});
	decal.texturePath = json.value("texturePath", std::string{});
	decal.position = ReadFloat3(json, "position", decal.position);
	decal.rotation = ReadFloat4(json, "rotation", decal.rotation);
	decal.scale = ReadFloat3(json, "scale", decal.scale);
	decal.opacity = json.value("opacity", decal.opacity);
	decal.normalThreshold = json.value("normalThreshold", decal.normalThreshold);
	decal.edgeSoftness = json.value("edgeSoftness", decal.edgeSoftness);

	decal.hasMaterialParameters = json.contains("materialParameters") && json["materialParameters"].is_object();
	if (decal.hasMaterialParameters)
	{
		for (const auto& [name, valueJson] : json["materialParameters"].items())
		{
			MAP_DECAL_PARAMETER_DATA parameter{};
			parameter.name = name;
			if (valueJson.is_number())
			{
				parameter.values.push_back(valueJson.get<_float>());
			}
			else if (valueJson.is_array())
			{
				for (const auto& value : valueJson)
				{
					if (value.is_number())
						parameter.values.push_back(value.get<_float>());
				}
			}
			decal.materialParameters.push_back(std::move(parameter));
		}
	}
	else
	{
		decal.legacyAlbedo = ReadFloat4(json, "albedo", decal.legacyAlbedo);
		decal.legacyEmissive = ReadFloat3(json, "emissive", decal.legacyEmissive);
		decal.legacyEmissiveIntensity = json.value("emissiveIntensity", decal.legacyEmissiveIntensity);
	}

	if (json.contains("textureOverrides") && json["textureOverrides"].is_array())
	{
		for (const auto& textureJson : json["textureOverrides"])
		{
			decal.textureOverrides.push_back(
			{
				textureJson.value("slot", 0u),
				textureJson.value("group", std::string{}),
				textureJson.value("tag", std::string{}),
				textureJson.value("path", std::string{})
			});
		}
	}

	return decal;
}

_float3 CMapChunkSerializer::ReadFloat3(const nlohmann::ordered_json& json, const char* key, const _float3& fallback) const
{
	if (!json.contains(key) || !json[key].is_array() || json[key].size() < 3)
		return fallback;

	const auto& value = json[key];

	return { value[0], value[1], value[2] };
}

_float4 CMapChunkSerializer::ReadFloat4(const nlohmann::ordered_json& json, const char* key, const _float4& fallback) const
{
	if (!json.contains(key) || !json[key].is_array() || json[key].size() < 4)
		return fallback;

	const auto& value = json[key];

	return { value[0], value[1], value[2], value[3] };
}
