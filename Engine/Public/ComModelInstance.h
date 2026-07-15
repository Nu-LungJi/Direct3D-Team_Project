

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

public:
	virtual void UpdateGUI() override;
private:
	explicit CComModelInstance();
	~CComModelInstance() override;


private:
	HRESULT Initialize(void* pArg) override;
	

public:
	HRESULT	Bind_BoneMatrices(ID3D11DeviceContext* pContext, uint32_t iMeshIndex);

	/*----------- 광윤 추가 -----------*/
	VOID Bind_Textures(ID3D11DeviceContext* pContext, uint32_t _MeshIndex);
	VOID Bind_Materials(ID3D11DeviceContext* pContext, _float3 _EmissiveColor, _float _EmissiveIntensity, _float _ObjectAlpha);
	/*---------------------------------*/

	SPtr<CResTexture2D>	Get_MeshTexture(uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex);
public:
	SPtr<CResModel> GetModel() { return m_pModel; }
	SPtr<const CResModel> GetModel() const { return m_pModel; }
	HRESULT ChangeModel(const StringID& sGroupTag, const StringID& sResTag);


	std::vector<_float4x4>&			Get_CombinedBoneMatrices() { return m_CombinedBoneMatrices; }

	StringID Get_GroupTag() { return m_sGroupTag; }
	StringID Get_ResTag() { return m_sResTag; }


private:
	SPtr<CResModel> m_pModel;
	SPtr<CResCBuffer> m_Buffer;
	std::vector<_float4x4> m_CombinedBoneMatrices;

	StringID m_sGroupTag;
	StringID m_sResTag;

public:
	static UPtr<CComModelInstance> Create();
	UPtr<CPrototype> Clone(void* pArg) override;


public:
	void DebugDraw_Bones(const _float4x4& WorldMatrix);

public:
	HRESULT Bind_GPUAnimationSRVs_CS(ID3D11DeviceContext* pContext);
	HRESULT Bind_GPUSkinBones_VS(ID3D11DeviceContext* pContext);

	void Unbind_GPUAnimationSRVs_CS(ID3D11DeviceContext* pContext);
private:
	bool m_bDebugBoneEdit = false;
	int  m_iDebugSelectedBone = 0;

	std::vector<_float3> m_DebugBoneLocalOffsets;

private:
	void EnsureDebugBoneOffsetSize();
	void ApplyDebugBoneLocalOffsets();


};

NS_END
