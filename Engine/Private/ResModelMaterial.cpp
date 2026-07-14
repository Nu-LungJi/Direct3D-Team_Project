#include "pch.h"
#include "ResModelMaterial.h"
#include "ResTexture2D.h"
#include <fstream>

NS_USING(Engine)

CResModelMaterial::CResModelMaterial(const _string& sPath)
	: CResource{ sPath }
{
}

CResModelMaterial::~CResModelMaterial()
{
}

HRESULT CResModelMaterial::Load(const std::any& arg)
{

	auto descArg = std::any_cast<DESC>(&arg);
	if (!descArg)
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}
	m_eState = STATE::LOADING;
	auto& strModelFilePath = m_sPath;
	auto pPoint = descArg->ptr;
	{
		std::filesystem::path textureBaseDir =
			MakeTextureBaseDirFromModelPath(strModelFilePath);

		m_iMaterialTypeNum = *(uint32_t*)pPoint;
		pPoint += sizeof(uint32_t);


		uint32_t textureTypeCount = *(uint32_t*)pPoint;
		pPoint += sizeof(uint32_t);

		

		for (size_t i = 0; i < textureTypeCount; i++)
		{
			uint32_t textureCount = *(uint32_t*)pPoint;
			pPoint += sizeof(uint32_t);

			for (size_t j = 0; j < textureCount; j++)
			{
				uint32_t textureType = *(uint32_t*)pPoint;
				pPoint += sizeof(uint32_t);

				uint32_t len = *(uint32_t*)pPoint;
				pPoint += sizeof(uint32_t);

				std::string file;
				file.resize(len);
				memcpy(file.data(), pPoint, len);
				pPoint += len;

				uint32_t extLen = *(uint32_t*)pPoint;
				pPoint += sizeof(uint32_t);

				std::string ext;
				ext.resize(extLen);
				memcpy(ext.data(), pPoint, extLen);
				pPoint += extLen;

				// ------------------------------------------------------------
				// 핵심:
				// Models 기준이 아니라 Textures 기준으로 텍스처 경로 생성
				// ------------------------------------------------------------
				std::filesystem::path texPath =
					textureBaseDir / (file + ext);

				std::filesystem::path ddsPath = texPath;
				ddsPath.replace_extension(".dds");

				// dds가 있으면 dds 우선
				if (std::filesystem::exists(ddsPath))
				{
					texPath = ddsPath;
				}

				_bool b_cache = false;

				if (auto pvecRes = CGameInstance::Get().GetResource("ONLY_MINSU_NO_TOUCH", texPath.string()))
				{
					if (pvecRes->empty())
					{
						b_cache = false;
					}
					else
					{
						b_cache = true;
					}
				}


				if (b_cache) {

					auto resTex = CGameInstance::Get().GetResourceFirst<CResTexture2D>("ONLY_MINSU_NO_TOUCH", texPath.string());
					
					m_Materials[textureType].push_back(resTex);
				}
				else {
					auto resTex = CResTexture2D::Create(texPath.string());
					CGameInstance::Get().AddResource("ONLY_MINSU_NO_TOUCH", texPath.string(), resTex);

					if (resTex == nullptr)
					{
						m_eState = STATE::LOADFAIL;
						return E_FAIL;
					}

					if (FAILED(resTex->Load()))
					{
						m_eState = STATE::LOADFAIL;
						return E_FAIL;
					}

					m_Materials[textureType].push_back(resTex);
				}

			
			}
		}

	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResModelMaterial::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}
HRESULT CResModelMaterial::LoadAssimp(aiMaterial* material, uint32_t materialNum)
{
	if (material == nullptr)
		return E_FAIL;

	if (m_eState == STATE::LOADED)
		return S_OK;

	m_eState = STATE::LOADING;

	m_iMaterialTypeNum = materialNum;

	// ------------------------------------------------------------
	// Models 기준이 아니라 Textures 기준으로 텍스처 폴더 생성
	// ------------------------------------------------------------
	std::filesystem::path textureBaseDir =
		MakeTextureBaseDirFromModelPath(m_sPath);

	for (UINT i = 0; i < AI_TEXTURE_TYPE_MAX; ++i)
	{
		const aiTextureType textureType = static_cast<aiTextureType>(i);
		const UINT textureCount = material->GetTextureCount(textureType);

		if (textureCount == 0)
			continue;

		for (UINT j = 0; j < textureCount; ++j)
		{
			aiString texturePath{};

			if (material->GetTexture(textureType, j, &texturePath) != AI_SUCCESS)
				continue;

			_char szFileName[MAX_PATH] = {};
			_char szExt[MAX_PATH] = {};

			_splitpath_s(
				texturePath.C_Str(),
				nullptr,
				0,
				nullptr,
				0,
				szFileName,
				MAX_PATH,
				szExt,
				MAX_PATH
			);

			std::filesystem::path texPath =
				textureBaseDir / (std::string(szFileName) + std::string(szExt));

			std::filesystem::path ddsPath = texPath;
			ddsPath.replace_extension(".dds");

			// 같은 이름의 dds가 있으면 dds 우선 사용
			if (std::filesystem::exists(ddsPath))
			{
				texPath = ddsPath;
			}



			auto resTex = CResTexture2D::Create(texPath.string());
			CGameInstance::Get().AddResource("ONLY_MINSU_NO_TOUCH", texPath.string(), resTex);

			if (resTex == nullptr)
			{
				m_eState = STATE::LOADFAIL;
				return E_FAIL;
			}

			if (FAILED(resTex->Load()))
			{
				m_eState = STATE::LOADFAIL;
				return E_FAIL;
			}

			m_Materials[i].push_back(resTex);
		}
	}

	m_eState = STATE::LOADED;
	return S_OK;
}
std::filesystem::path CResModelMaterial::MakeTextureBaseDirFromModelPath(
	const std::string& modelFilePath
)
{
	std::filesystem::path modelPath(modelFilePath);
	std::filesystem::path modelDir = modelPath.parent_path();

	std::filesystem::path textureDir;

	bool replacedModels = false;
	bool hasOriginData = false;

	for (const auto& part : modelDir)
	{
		std::string token = part.string();

		if (!replacedModels && _stricmp(token.c_str(), "Models") == 0)
		{
			textureDir /= "Textures";
			replacedModels = true;
			continue;
		}

		// ------------------------------------------------------------
		// OriginData는 제거하지 않고 그대로 유지
		// Models/OriginData/Static
		// -> Textures/OriginData/Static
		// ------------------------------------------------------------
		if (_stricmp(token.c_str(), "OriginData") == 0)
		{
			hasOriginData = true;
		}

		textureDir /= part;
	}

	// ------------------------------------------------------------
	// OriginData에서 직접 로드하는 경우
	//
	// Models/OriginData/Static/HorseStatue.fbx
	// -> Textures/OriginData/Static/
	//
	// 여기서는 HorseStatue 붙이지 않음
	// ------------------------------------------------------------
	if (hasOriginData)
	{
		return textureDir;
	}

	// ------------------------------------------------------------
	// bin 로드하는 경우
	//
	// Models/Static/SM_HorseStatue.bin
	// -> Textures/Static/HorseStatue/
	// ------------------------------------------------------------
	if (_stricmp(modelDir.filename().string().c_str(), "Static") == 0)
	{
		std::string stem = modelPath.stem().string();

		if (stem.rfind("SM_", 0) == 0)
			stem = stem.substr(3);
		else if (stem.rfind("SK_", 0) == 0)
			stem = stem.substr(3);

		textureDir /= stem;
	}

	return textureDir;
}
SPtr<CResModelMaterial> CResModelMaterial::Create(const _string& sPath)
{
	return ToSPtr(new CResModelMaterial{ sPath });
}

void CResModelMaterial::Free()
{
	std::unordered_set<_string> cacheKeys;
	for (auto& textures : m_Materials)
	{
		for (auto& texture : textures)
		{
			if (texture)
				cacheKeys.insert(texture->GetPath());
		}
		textures.clear();
	}

	for (auto& textures : m_Materials)
	{

		for (auto& texture : textures)
		{
			if (const auto* cached = CGameInstance::Get().GetResource(
				"ONLY_MINSU_NO_TOUCH", texture->GetPath());
				cached && !cached->empty())
			{
				// manager 1 + 이 material의 texture 1일 때만 제거
				if (texture.use_count() == 2)
				{
					CGameInstance::Get().DelResource(
						"ONLY_MINSU_NO_TOUCH",
						texture->GetPath());
				}
			}
		}
		
	}



	// The material's texture references are gone now.  Only evict a cache
	// entry when it is its sole remaining owner.
	for (const auto& cacheKey : cacheKeys)
	{
		const auto* cached = CGameInstance::Get().GetResource(
			"ONLY_MINSU_NO_TOUCH", cacheKey);

		if (cached && cached->size() == 1 && (*cached)[0] && (*cached)[0].use_count() == 1)
		{
			CGameInstance::Get().DelResource("ONLY_MINSU_NO_TOUCH", cacheKey);
		}
	}

	__super::Free();

}
