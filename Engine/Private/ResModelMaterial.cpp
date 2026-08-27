#include "pch.h"
#include "ResModelMaterial.h"
#include "ResTexture2D.h"
#include <array>
#include <fstream>
#include <mutex>
#include <unordered_map>

NS_USING(Engine)

std::mutex& CResModelMaterial::GetTextureLoadMutex(const std::string& texturePath)
{
	constexpr size_t TEXTURE_LOAD_LOCK_COUNT = 64;
	static std::array<std::mutex, TEXTURE_LOAD_LOCK_COUNT> textureLoadLocks{};
	return textureLoadLocks[std::hash<std::string>{}(texturePath) % TEXTURE_LOAD_LOCK_COUNT];
}

std::mutex& CResModelMaterial::GetTextureIndexMutex()
{
	static std::mutex textureIndexMutex{};
	return textureIndexMutex;
}

CResModelMaterial::TEXTURE_ROOT_INDEXES& CResModelMaterial::GetTextureRootIndexes()
{
	static TEXTURE_ROOT_INDEXES textureRootIndexes{};
	return textureRootIndexes;
}

std::string CResModelMaterial::LowerPathString(const std::filesystem::path& path)
{
	std::string value = path.lexically_normal().generic_string();
	std::transform(value.begin(), value.end(), value.begin(),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	return value;
}

std::filesystem::path CResModelMaterial::FindTextureRoot(const std::filesystem::path& modelPath)
{
	std::filesystem::path result{};
	for (const auto& part : modelPath.parent_path())
	{
		if (_stricmp(part.string().c_str(), "Models") == 0)
		{
			result /= "Textures";
			return result;
		}
		result /= part;
	}
	return {};
}

CResModelMaterial::TEXTURE_PATH_INDEX CResModelMaterial::BuildTexturePathIndex(const std::filesystem::path& textureRoot)
{
	TEXTURE_PATH_INDEX index{};
	std::error_code ec{};
	for (const auto& entry : std::filesystem::recursive_directory_iterator(
		textureRoot, std::filesystem::directory_options::skip_permission_denied, ec))
	{
		if (ec)
			break;
		if (!entry.is_regular_file(ec))
			continue;
		index[LowerPathString(entry.path().filename())].push_back(entry.path());
	}
	return index;
}

void CResModelMaterial::WarmUpTextureSearchIndex(
	const std::filesystem::path& modelRoot)
{
	const std::filesystem::path textureRoot = FindTextureRoot(modelRoot / "placeholder.bin");
	if (textureRoot.empty())
		return;

	const std::string rootKey = LowerPathString(textureRoot);
	{
		std::lock_guard<std::mutex> lock(GetTextureIndexMutex());
		if (GetTextureRootIndexes().contains(rootKey))
			return;
	}

	// 디렉터리 순회는 락 밖에서 수행하여 이미 생성된 다른 루트의 조회를 막지 않는다.
	TEXTURE_PATH_INDEX newIndex = BuildTexturePathIndex(textureRoot);
	std::lock_guard<std::mutex> lock(GetTextureIndexMutex());
	GetTextureRootIndexes().try_emplace(rootKey, std::move(newIndex));
}

std::filesystem::path CResModelMaterial::ResolveModelTexture(
	const std::filesystem::path& modelPath,
	const std::filesystem::path& preferredDirectory,
	const std::string& file,
	const std::string& extension)
{
	const auto existingIn = [&](const std::filesystem::path& directory)
	{
		std::filesystem::path original = directory / (file + extension);
		std::filesystem::path dds = original;
		dds.replace_extension(".dds");
		if (std::filesystem::exists(dds)) return dds;
		if (std::filesystem::exists(original)) return original;
		return std::filesystem::path{};
 	};
	if (auto found = existingIn(preferredDirectory); !found.empty())
		return found;

	const std::filesystem::path textureRoot = FindTextureRoot(modelPath);
	if (textureRoot.empty())
		return preferredDirectory / (file + extension);

	const std::string rootKey = LowerPathString(textureRoot);
	// 일반 모델 로드가 선행된 경우에도 기존 동작을 유지한다.
	// 스트리밍 맵에서는 SetResourceIndex 시점에 이미 생성되어 즉시 반환한다.
	WarmUpTextureSearchIndex(modelPath.parent_path());

	std::filesystem::path desired = file + extension;
	desired.replace_extension(".dds");
	std::lock_guard<std::mutex> lock(GetTextureIndexMutex());
	const auto rootIter = GetTextureRootIndexes().find(rootKey);
	if (rootIter == GetTextureRootIndexes().end())
		return preferredDirectory / desired.filename();

	auto candidates = rootIter->second.find(LowerPathString(desired.filename()));
	if (candidates == rootIter->second.end())
	{
		const std::filesystem::path originalName = file + extension;
		candidates = rootIter->second.find(LowerPathString(originalName.filename()));
	}
	if (candidates == rootIter->second.end() || candidates->second.empty())
		return preferredDirectory / desired.filename();

	std::string modelName = modelPath.stem().string();
	if (modelName.rfind("SM_", 0) == 0 || modelName.rfind("SK_", 0) == 0)
		modelName = modelName.substr(3);
	const auto exactModelFolder = std::find_if(
		candidates->second.begin(), candidates->second.end(),
		[&](const auto& candidate)
		{
			return _stricmp(candidate.parent_path().filename().string().c_str(),
				modelName.c_str()) == 0;
		});
	if (exactModelFolder != candidates->second.end())
		return *exactModelFolder;
	return *std::min_element(candidates->second.begin(), candidates->second.end());
}

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
				std::filesystem::path texPath = descArg->recursiveTextureSearch
					? ResolveModelTexture(strModelFilePath, textureBaseDir, file, ext)
					: textureBaseDir / (file + ext);

				std::filesystem::path ddsPath = texPath;
				ddsPath.replace_extension(".dds");

				// dds가 있으면 dds 우선
				if (std::filesystem::exists(ddsPath))
				{
					texPath = ddsPath;
				}

				// 일부 캐릭터 모델에는 원본 머티리얼이 참조하지만 배포 리소스에는
				// 포함되지 않은 선택 텍스처가 있다. 비어 있는 슬롯은 렌더링 시
				// 기본 텍스처로 대체되므로 모델 전체 로딩을 실패시키지 않는다.
				if (!std::filesystem::exists(texPath))
					continue;

				const _string texturePath = texPath.string();
				std::lock_guard<std::mutex> textureLoadLock(GetTextureLoadMutex(texturePath));
				_bool isCreated = false;
				auto resTex = CGameInstance::Get().GetOrCreateResourceByPath<CResTexture2D>(
					texturePath,
					[&]()
					{
						isCreated = true;
						return CResTexture2D::Create(texturePath);
					});

				if (!resTex)
				{
					m_eState = STATE::LOADFAIL;
					return E_FAIL;
				}

				if (isCreated && FAILED(resTex->Load()))
				{
					m_eState = STATE::LOADFAIL;
					return E_FAIL;
				}

				m_Materials[textureType].push_back(resTex);


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

			std::filesystem::path texPath = ResolveModelTexture(
				m_sPath, textureBaseDir, szFileName, szExt);

			std::filesystem::path ddsPath = texPath;
			ddsPath.replace_extension(".dds");

			// 같은 이름의 dds가 있으면 dds 우선 사용
			if (std::filesystem::exists(ddsPath))
			{
				texPath = ddsPath;
			}



			const _string resolvedTexturePath = texPath.string();
			std::lock_guard<std::mutex> textureLoadLock(GetTextureLoadMutex(resolvedTexturePath));
			_bool isCreated = false;
			auto resTex = CGameInstance::Get().GetOrCreateResourceByPath<CResTexture2D>(
				resolvedTexturePath,
				[&]()
				{
					isCreated = true;
					return CResTexture2D::Create(resolvedTexturePath);
				});

			if (resTex == nullptr)
			{
				m_eState = STATE::LOADFAIL;
				return E_FAIL;
			}

			if (isCreated && FAILED(resTex->Load()))
			{
				m_eState = STATE::LOADFAIL;
				return E_FAIL;
			}

			m_Materials[i].emplace_back(resTex);
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
	//std::unordered_set<_string> cacheKeys;

	//// [수정 1] 분리되어 있던 두 루프를 하나로 합침 (Dead Code 버그 해결)
	//for (auto& textures : m_Materials)
	//{
	//	for (auto& texture : textures)
	//	{
	//		if (texture)
	//		{
	//			_string path = texture->GetPath();
	//			cacheKeys.insert(path);

	//			// [수정 2] 값 반환으로 인한 use_count 증가를 피하기 위해, 
	//			// GetResource를 호출하기 *전*에 현재 텍스처의 카운트를 확인합니다.
	//			// (현재 Material 1 + Manager 캐시 1 = 총 2인 상태)
	//			if (texture.use_count() == 2)
	//			{
	//				// 이제 값으로 리턴받습니다. (이 순간 복사가 발생해 내부 카운트는 3이 됩니다)
	//				auto cached = CGameInstance::Get().GetResource("ONLY_MINSU_NO_TOUCH", path);

	//				if (!cached.empty())
	//				{
	//					// [수정 3] 매니저 내부에서 텍스처가 파괴되며 발생하는 데드락(멈춤) 방지
	//					auto keepAlive = texture;
	//					CGameInstance::Get().DelResource("ONLY_MINSU_NO_TOUCH", path);
	//				}
	//			}
	//		}
	//	}
	//	// 모든 검사가 끝난 후 현재 재질의 텍스처 레퍼런스 제거
	//	textures.clear();
	//}

	//// The material's texture references are gone now.
	//// Only evict a cache entry when it is its sole remaining owner.
	//for (const auto& cacheKey : cacheKeys)
	//{
	//	// 값으로 리턴받습니다. (Manager 캐시 1 + 현재 지역변수 cached 1 = 총 2)
	//	auto cached = CGameInstance::Get().GetResource("ONLY_MINSU_NO_TOUCH", cacheKey);

	//	if (cached.size() == 1 && cached[0])
	//	{
	//		// [수정 4] Manager에만 유일하게 남은 상태라면, 
	//		// Manager(1) + 방금 값으로 복사받은 cached[0](1) = 총 2여야 정상입니다.
	//		// (기존 코드의 1이 아닌 2로 체크해야 합니다)
	//		if (cached[0].use_count() == 2)
	//		{
	//			auto keepAlive = cached[0]; // 데드락 방지
	//			CGameInstance::Get().DelResource("ONLY_MINSU_NO_TOUCH", cacheKey);
	//		}
	//	}
	//}

	__super::Free();
}

//void CResModelMaterial::Free()
//{
//	std::unordered_set<_string> cacheKeys;
//	for (auto& textures : m_Materials)
//	{
//		for (auto& texture : textures)
//		{
//			if (texture)
//				cacheKeys.insert(texture->GetPath());
//		}
//		textures.clear();
//	}
//
//	for (auto& textures : m_Materials)
//	{
//
//		for (auto& texture : textures)
//		{
//			if (const auto* cached = CGameInstance::Get().GetResource(
//				"ONLY_MINSU_NO_TOUCH", texture->GetPath());
//				cached && !cached->empty())
//			{
//				// manager 1 + 이 material의 texture 1일 때만 제거
//				if (texture.use_count() == 2)
//				{
//					CGameInstance::Get().DelResource(
//						"ONLY_MINSU_NO_TOUCH",
//						texture->GetPath());
//				}
//			}
//		}
//		
//	}
//
//
//
//	// The material's texture references are gone now.  Only evict a cache
//	// entry when it is its sole remaining owner.
//	for (const auto& cacheKey : cacheKeys)
//	{
//		const auto* cached = CGameInstance::Get().GetResource(
//			"ONLY_MINSU_NO_TOUCH", cacheKey);
//
//		if (cached && cached->size() == 1 && (*cached)[0] && (*cached)[0].use_count() == 1)
//		{
//			CGameInstance::Get().DelResource("ONLY_MINSU_NO_TOUCH", cacheKey);
//		}
//	}
//
//	__super::Free();
//
//}
