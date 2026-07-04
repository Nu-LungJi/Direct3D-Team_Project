

#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class CResModelMesh;
class CResModelMaterial;
class CResModelBone;
class CResModelMaterial;
class CResModel;

class ENGINE_DLL CComModelInstance : public CComponent
{
public:
	typedef struct tagDesc : CComponent::DESC
	{
		StringID sGroupTag;
		StringID sResTag;
	}DESC;
public:
	DECLARE_DERIVED_TYPE(CComModelInstance, CComponent)



private:
	explicit CComModelInstance();
	~CComModelInstance() override;


private:
	HRESULT Initialize(void* pArg) override;
	

public:
	HRESULT	Bind_BoneMatrices(ID3D11DeviceContext* pContext, uint32_t iMeshIndex);
	HRESULT Bind_Materials(ID3D11DeviceContext* pContext, uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex);
public:
	SPtr<CResModel> GetModel() { return m_pModel; }	

private:
	SPtr<CResModel> m_pModel;
public:
	static UPtr<CComModelInstance> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END