#include "pch.h"
#include "MapMaterialRepository.h"
#include <fstream>

NS_USING(Engine)

HRESULT CMapMaterialRepository::SaveFile(const std::filesystem::path& filePath, const MATERIAL_MAP& materials) const
{
	try
	{
		std::error_code error;
		std::filesystem::create_directories(filePath.parent_path(), error);
		if (error)
			return E_FAIL;

		nlohmann::ordered_json rootJson = nlohmann::ordered_json::object();
		rootJson["Version"] = 1;
		rootJson["Materials"] = nlohmann::ordered_json::object();

		for (const auto& [modelName, material] : materials)
		{
			rootJson["Materials"][modelName] = WriteMaterial(material);
		}

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

HRESULT CMapMaterialRepository::LoadFile(const std::filesystem::path& filePath)
{
	Clear();
	if (!std::filesystem::exists(filePath))
		return S_OK;

	try
	{
		std::ifstream inFile(filePath);
		if (!inFile.is_open())
			return E_FAIL;

		nlohmann::ordered_json rootJson;
		inFile >> rootJson;
		if (!rootJson.contains("Materials") || !rootJson["Materials"].is_object())
			return E_FAIL;

		MATERIAL_MAP loadedMaterials;
		for (const auto& [modelName, materialJson] : rootJson["Materials"].items())
		{
			loadedMaterials[modelName] = ReadMaterial(materialJson);
		}

		{
			std::unique_lock lock(m_Mutex);
			m_Materials = std::move(loadedMaterials);
		}

		return S_OK;
	}
	catch (const std::exception&)
	{
		return E_FAIL;
	}
}

MATERIAL_DESC CMapMaterialRepository::Find(const std::string& modelName) const
{
	std::shared_lock lock(m_Mutex);
	const auto iter = m_Materials.find(modelName);

	return iter != m_Materials.end() ? iter->second : MATERIAL_DESC{};
}

CMapMaterialRepository::MATERIAL_MAP CMapMaterialRepository::GetSnapshot() const
{
	std::shared_lock lock(m_Mutex);
	return m_Materials;
}

void CMapMaterialRepository::Clear()
{
	std::unique_lock lock(m_Mutex);
	m_Materials.clear();
}

nlohmann::ordered_json CMapMaterialRepository::WriteMaterial(const MATERIAL_DESC& material) const
{
	const _float3 emissiveColor = material.m_fEmissiveColor;
	return
	{
		{ "NormalIntensity", material.m_fNormalIntensity },
		{ "MetallicIntensity", material.m_fMetallicIntensity },
		{ "RoughnessIntensity", material.m_fRoughnessIntensity },
		{ "AmbientIntensity", material.m_fAmbientIntensity },
		{ "EmissiveColor", { emissiveColor.x, emissiveColor.y, emissiveColor.z } },
		{ "EmissiveIntensity", material.m_fEmissiveIntensity },
		{ "ObjectAlpha", material.m_fObjectAlpha }
	};
}

MATERIAL_DESC CMapMaterialRepository::ReadMaterial(const nlohmann::ordered_json& json) const
{
	MATERIAL_DESC material{};
	if (!json.is_object())
		return material;

	material.m_fNormalIntensity = json.value("NormalIntensity", material.m_fNormalIntensity);
	material.m_fMetallicIntensity = json.value("MetallicIntensity", material.m_fMetallicIntensity);
	material.m_fRoughnessIntensity = json.value("RoughnessIntensity", material.m_fRoughnessIntensity);
	material.m_fAmbientIntensity = json.value("AmbientIntensity", material.m_fAmbientIntensity);
	material.m_fEmissiveColor = ReadFloat3(json, "EmissiveColor", material.m_fEmissiveColor);
	material.m_fEmissiveIntensity = json.value("EmissiveIntensity", material.m_fEmissiveIntensity);
	material.m_fObjectAlpha = json.value("ObjectAlpha", material.m_fObjectAlpha);

	return material;
}

_float3 CMapMaterialRepository::ReadFloat3(const nlohmann::ordered_json& json, const char* key, const _float3& fallback) const
{
	if (!json.contains(key) || !json[key].is_array() || json[key].size() < 3)
		return fallback;

	const auto& value = json[key];

	return { value[0], value[1], value[2] };
}
