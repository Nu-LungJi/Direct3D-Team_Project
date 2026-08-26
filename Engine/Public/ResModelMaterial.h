
#pragma once

#include "Resource.h"
#include <mutex>
#include <unordered_map>


NS_BEGIN(Engine)


class ENGINE_DLL CResModelMaterial final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResModelMaterial, CResource)
public:
	typedef struct tagDesc {
		_char* ptr;
		_bool recursiveTextureSearch{ false };
	}DESC;
private:
	explicit CResModelMaterial(const _string& sPath);
	~CResModelMaterial() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	// 스트리밍 워커가 첫 텍스처를 찾을 때 디렉터리 전체를 순회하지 않도록
	// 모델 루트에 대응하는 텍스처 검색 인덱스를 미리 구축한다.
	static void WarmUpTextureSearchIndex(const std::filesystem::path& modelRoot);

	HRESULT LoadAssimp(aiMaterial* material, uint32_t materialNum);
	std::filesystem::path MakeTextureBaseDirFromModelPath(const std::string& modelFilePath);
public:
	std::vector<SPtr<CResTexture2D>>* GetTextures() { return m_Materials; }
	uint32_t GetMaterialSize() const { return ENG_AI_TEXTURE_TYPE_MAX; }
	uint32_t GetMaterialTypeNum() { return m_iMaterialTypeNum; }


public:

private:
	using TEXTURE_PATH_INDEX =
		std::unordered_map<std::string, std::vector<std::filesystem::path>>;
	using TEXTURE_ROOT_INDEXES =
		std::unordered_map<std::string, TEXTURE_PATH_INDEX>;

	static std::mutex& GetTextureLoadMutex(const std::string& texturePath);
	static std::mutex& GetTextureIndexMutex();
	static TEXTURE_ROOT_INDEXES& GetTextureRootIndexes();
	static std::string LowerPathString(const std::filesystem::path& path);
	static std::filesystem::path FindTextureRoot(const std::filesystem::path& modelPath);
	static TEXTURE_PATH_INDEX BuildTexturePathIndex(const std::filesystem::path& textureRoot);
	static std::filesystem::path ResolveModelTexture(
		const std::filesystem::path& modelPath,
		const std::filesystem::path& preferredDirectory,
		const std::string& file,
		const std::string& extension);

	HRESULT AcquireCachedTexture(const _string& texturePath, SPtr<CResTexture2D>& outTexture);

	uint32_t			m_iMaterialTypeNum{};

private:
	std::vector<SPtr<CResTexture2D>>			m_Materials[ENG_AI_TEXTURE_TYPE_MAX];
	
public:
	static SPtr<CResModelMaterial> Create(const _string& sPath = {});

private:
	void Free() override;

};

NS_END
