

#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class CResTestModelMesh;
class CResTestModelMaterial;
class CResTestModelBone;
class CResTestModelMaterial;
class CResTestModel;

class ENGINE_DLL CComModelInstance : public CComponent
{
public:
	typedef struct tagDesc : CComponent::DESC
	{
		SPtr<CResTestModel>	pModel;
	}DESC;
public:
	DECLARE_DERIVED_TYPE(CComModelInstance, CComponent)



private:
	explicit CComModelInstance();
	~CComModelInstance() override;


private:
	HRESULT Initialize(void* pArg) override;
	

public:
	HRESULT	Bind_BoneMatrices(uint32_t iMeshIndex);
	HRESULT Bind_Materials(uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex);
public:
	SPtr<CResTestModel> GetModel() { return m_pModel; }	

private:
	SPtr<CResTestModel> m_pModel;
public:
	static UPtr<CComModelInstance> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END