#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class CResStaticModelMesh;
class CResModelMaterial;
class CResModelBone;
class CResModelMaterial;
class CResStaticModel;
class CResTexture2D;

class ENGINE_DLL CComStaticModelInstance : public CComponent
{
public:
	typedef struct tagDesc : CComponent::DESC
	{
		StringID sGroupTag;
		StringID sResTag;
	}DESC;
public:
	DECLARE_DERIVED_TYPE(CComStaticModelInstance, CComponent)



private:
	explicit CComStaticModelInstance();
	~CComStaticModelInstance() override;


private:
	HRESULT Initialize(void* pArg) override;


public:
	HRESULT Bind_Materials(ID3D11DeviceContext* pContext, uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex);
	SPtr<CResTexture2D> Get_MeshTexture(uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex);

public:
	SPtr<CResStaticModel> GetModel() { return m_pModel; }
	HRESULT ChangeModel(const StringID& sGroupTag, const StringID& sResTag);
private:
	SPtr<CResStaticModel> m_pModel;

public:
	static UPtr<CComStaticModelInstance> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END