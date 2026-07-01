

#pragma once

#include "Resource.h"
struct aiMaterial;

NS_BEGIN(Engine)

class ENGINE_DLL CResTestModelMaterial final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResTestModelMaterial, CResource)
public:
	typedef struct tagDesc {
		const aiMaterial* pAIMaterial;
	}DESC;
private:
	explicit CResTestModelMaterial(const _string& sPath);
	~CResTestModelMaterial() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

public:
	std::vector<SPtr<CResTexture2D>>* GetTextures() { return m_Materials; }
	uint32_t GetMaterialSize() const { return ENG_AI_TEXTURE_TYPE_MAX ; }
	HRESULT Bind_ShaderResource( AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex);
private:
	std::vector<SPtr<CResTexture2D>>			m_Materials[ENG_AI_TEXTURE_TYPE_MAX];
public:
	static SPtr<CResTestModelMaterial> Create(const _string& sPath = {});
};

NS_END