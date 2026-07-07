
#pragma once

#include "Resource.h"


NS_BEGIN(Engine)


class ENGINE_DLL CResModelMaterial final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResModelMaterial, CResource)
public:
	typedef struct tagDesc {
		_char* ptr;
	}DESC;
private:
	explicit CResModelMaterial(const _string& sPath);
	~CResModelMaterial() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

public:
	std::vector<SPtr<CResTexture2D>>* GetTextures() { return m_Materials; }
	uint32_t GetMaterialSize() const { return ENG_AI_TEXTURE_TYPE_MAX; }
	uint32_t GetMaterialTypeNum() { return m_iMaterialTypeNum; }


public:

private:
	uint32_t			m_iMaterialTypeNum{};

private:
	std::vector<SPtr<CResTexture2D>>			m_Materials[ENG_AI_TEXTURE_TYPE_MAX];
public:
	static SPtr<CResModelMaterial> Create(const _string& sPath = {});
};

NS_END