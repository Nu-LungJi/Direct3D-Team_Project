#include "pch.h"
#include "DecalMaterial.h"
#include "GameInstance.h"
#include "Resources.h"
#include <fstream>

NS_USING(Engine)

namespace
{
	std::mutex g_DecalMaterialMutex{};
	std::unordered_map<_string, WPtr<CDecalMaterial>> g_DecalMaterialCache{};

	_string NormalizePath(const _string& path)
	{
		return std::filesystem::path{ path }.lexically_normal().generic_string();
	}

	DECAL_PARAMETER_TYPE ParseParameterType(const _string& value)
	{
		if (value == "float2") return DECAL_PARAMETER_TYPE::FLOAT2;
		if (value == "float3") return DECAL_PARAMETER_TYPE::FLOAT3;
		if (value == "float4") return DECAL_PARAMETER_TYPE::FLOAT4;
		if (value == "color3") return DECAL_PARAMETER_TYPE::COLOR3;
		if (value == "color4") return DECAL_PARAMETER_TYPE::COLOR4;
		return DECAL_PARAMETER_TYPE::FLOAT;
	}

	uint32_t ParameterCount(DECAL_PARAMETER_TYPE type)
	{
		switch (type)
		{
		case DECAL_PARAMETER_TYPE::FLOAT2: return 2;
		case DECAL_PARAMETER_TYPE::FLOAT3:
		case DECAL_PARAMETER_TYPE::COLOR3: return 3;
		case DECAL_PARAMETER_TYPE::FLOAT4:
		case DECAL_PARAMETER_TYPE::COLOR4: return 4;
		default: return 1;
		}
	}

	void ReadDefaultValue(const nlohmann::json& value, _float* destination, uint32_t count)
	{
		if (value.is_number())
		{
			destination[0] = value.get<_float>();
			return;
		}
		if (!value.is_array())
			return;
		for (uint32_t i = 0; i < count && i < value.size(); ++i)
			if (value[i].is_number())
				destination[i] = value[i].get<_float>();
	}
}

CDecalMaterial::CDecalMaterial(_string path)
	: m_Path{ NormalizePath(path) }
{
}

HRESULT CDecalMaterial::Load()
{
	std::ifstream stream{ m_Path };
	if (!stream.is_open())
		return E_FAIL;

	nlohmann::json json{};
	try
	{
		stream >> json;
	}
	catch (const nlohmann::json::exception&)
	{
		return E_FAIL;
	}

	m_Name = json.value("name", std::filesystem::path{ m_Path }.stem().string());
	const _string shaderPath = NormalizePath(json.value("pixelShader", _string{}));
	if (shaderPath.empty())
		return E_FAIL;

	auto& gameInstance = CGameInstance::Get();
	m_PixelShader = gameInstance.GetOrCreateResourceByPath<CResPixelShader>(shaderPath, [&]()
	{
		auto shader = CResPixelShader::Create(shaderPath);
		if (!shader || FAILED(shader->Load()))
			return SPtr<CResPixelShader>{};
		return shader;
	});
	if (!m_PixelShader)
		return E_FAIL;

	m_DefaultParameters.fill(0.f);
	m_Parameters.clear();
	if (json.contains("parameters") && json["parameters"].is_array())
	{
		for (const auto& parameterJson : json["parameters"])
		{
			PARAMETER_DESC parameter{};
			parameter.name = parameterJson.value("name", _string{});
			parameter.type = ParseParameterType(parameterJson.value("type", _string{ "float" }));
			parameter.offset = parameterJson.value("offset", 0u);
			parameter.count = ParameterCount(parameter.type);
			parameter.minValue = parameterJson.value("min", 0.f);
			parameter.maxValue = parameterJson.value("max", 1.f);
			parameter.speed = parameterJson.value("speed", 0.01f);
			if (parameter.name.empty() || parameter.offset + parameter.count > PARAMETER_FLOAT_COUNT)
				return E_FAIL;
			if (parameterJson.contains("default"))
				ReadDefaultValue(parameterJson["default"], m_DefaultParameters.data() + parameter.offset, parameter.count);
			m_Parameters.push_back(std::move(parameter));
		}
	}

	m_Textures.clear();
	if (json.contains("textures") && json["textures"].is_array())
	{
		for (const auto& textureJson : json["textures"])
		{
			TEXTURE_DESC textureDesc{};
			textureDesc.name = textureJson.value("name", _string{});
			textureDesc.slot = textureJson.value("slot", TEXTURE_SLOT_BEGIN);
			textureDesc.path = NormalizePath(textureJson.value("path", _string{}));
			if (textureDesc.slot < TEXTURE_SLOT_BEGIN || textureDesc.slot > TEXTURE_SLOT_END || textureDesc.path.empty())
				return E_FAIL;

			textureDesc.texture = gameInstance.GetOrCreateResourceByPath<CResTexture2D>(textureDesc.path, [&]()
			{
				auto texture = CResTexture2D::Create(textureDesc.path);
				if (!texture || FAILED(texture->Load()))
					return SPtr<CResTexture2D>{};
				return texture;
			});
			if (!textureDesc.texture)
				return E_FAIL;
			m_Textures.push_back(std::move(textureDesc));
		}
	}

	return S_OK;
}

HRESULT CDecalMaterial::Bind(ID3D11DeviceContext* context) const
{
	if (!context || !m_PixelShader)
		return E_FAIL;
	context->PSSetShader(m_PixelShader->GetPixelShader().Get(), nullptr, 0);
	for (const auto& texture : m_Textures)
	{
		ID3D11ShaderResourceView* srv = texture.texture ? texture.texture->GetSRV().Get() : nullptr;
		context->PSSetShaderResources(texture.slot, 1, &srv);
	}
	return S_OK;
}

void CDecalMaterial::Unbind(ID3D11DeviceContext* context) const
{
	if (!context)
		return;
	ID3D11ShaderResourceView* nullSRV = nullptr;
	for (const auto& texture : m_Textures)
		context->PSSetShaderResources(texture.slot, 1, &nullSRV);
}

const CDecalMaterial::PARAMETER_DESC* CDecalMaterial::FindParameter(const _string& name) const
{
	const auto iter = std::find_if(m_Parameters.begin(), m_Parameters.end(), [&](const PARAMETER_DESC& parameter)
	{
		return parameter.name == name;
	});
	return iter == m_Parameters.end() ? nullptr : &*iter;
}

SPtr<CDecalMaterial> CDecalMaterial::LoadShared(const _string& path)
{
	const _string normalizedPath = NormalizePath(path);
	std::scoped_lock lock{ g_DecalMaterialMutex };
	if (const auto iter = g_DecalMaterialCache.find(normalizedPath); iter != g_DecalMaterialCache.end())
		if (auto material = iter->second.lock())
			return material;

	auto material = SPtr<CDecalMaterial>{ new CDecalMaterial{ normalizedPath } };
	if (FAILED(material->Load()))
		return nullptr;
	g_DecalMaterialCache[normalizedPath] = material;
	return material;
}

std::vector<_string> CDecalMaterial::FindMaterialFiles(const _string& rootPath)
{
	std::vector<_string> files{};
	const std::filesystem::path root{ rootPath };
	if (!std::filesystem::exists(root))
		return files;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
		if (entry.is_regular_file() && _stricmp(entry.path().extension().string().c_str(), ".json") == 0)
			files.push_back(entry.path().lexically_normal().generic_string());
	std::sort(files.begin(), files.end());
	return files;
}
