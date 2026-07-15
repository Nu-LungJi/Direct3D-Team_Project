
#pragma once

#include "Resource.h"
#include "ResModelBone.h"
#include "ResModelMesh.h"
#include "ResModelMaterial.h"
#include "ResModelAnim.h"


NS_BEGIN(Engine)

class ENGINE_DLL CResModel final : public CResource
{

public:
	DECLARE_DERIVED_TYPE(CResModel, CResource)
public:
	typedef struct tagDesc {
		_matrix PreTransformMatrix;
	}DESC;
private:
	explicit CResModel(const _string& sPath);
	~CResModel() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;


private:
	HRESULT Ready_Bones(_char* ptr);
	HRESULT Ready_Materials(const _string& strModelFilePath, _char* ptr);
	HRESULT Ready_Meshes(_char* ptr);
	HRESULT Ready_Animation();


	// for GPU
	HRESULT Ready_GPU_Ready();
	HRESULT Ready_GPU_Bone();

	HRESULT Ready_GPU_Animation();
	HRESULT Ready_GPU_MeshSkin();
	// GPU에서 읽기 위해 계산 순서 정해주는거
	HRESULT Ready_BoneDepths();

	HRESULT Calculate_BoneDepth(uint32_t iBoneIndex, std::vector<int32_t>& depthCache, std::vector<bool>& visiting, uint32_t& outDepth);


public:
	uint32_t Get_NumMeshes( ) const { return m_iNumMeshes;}


	int32_t Get_BoneIndex(const _char* pBoneName);

	const _float4x4* Get_BoneMatrixPtr(const _char* pBoneName);

	const _float4x4& Get_PreTransformMatrix() { return m_PreTransformMatrix; }



public:
	std::vector<SPtr<CResModelMesh>>& GetMeshes() { return m_Meshes; }
	std::vector<SPtr<CResModelMaterial>>& GetMaterials() { return m_Materials; }
	std::vector<SPtr<CResModelAnim>>& GetAnimations() { return m_Animations; }
	std::vector<SPtr<CResModelBone>>& GetBones() { return m_Bones; }

	ID3D11ShaderResourceView* Get_GPUBoneSRV() const{return m_pGPUBones->GetSRV().Get();}

	ID3D11ShaderResourceView* Get_GPUAnimationSRV() const{return m_pGPUAnimations->GetSRV().Get();}

	ID3D11ShaderResourceView* Get_GPUChannelSRV() const{return m_pGPUChannels->GetSRV().Get();}

	ID3D11ShaderResourceView* Get_GPUKeyFrameSRV() const{return  m_pGPUKeyFrames->GetSRV().Get();}

	ID3D11ShaderResourceView* Get_GPUBoneChannelMapSRV() const {return m_pGPUBoneChannelMap->GetSRV().Get();}

	ID3D11ShaderResourceView* Get_GPUSkinBoneSRV() const {return m_pGPUSkinBones->GetSRV().Get();}
	const GPU_MESH_SKIN_RANGE& Get_GPUMeshSkinRange(uint32_t iMeshIndex) const { return m_GPUMeshSkinRanges.at(iMeshIndex); }
protected:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};


private:
	uint32_t	m_iAnimCnt{};


private:
	MODEL						m_eModelType = {};
	uint32_t					m_iNumMeshes = {};
	std::vector<SPtr<CResModelMesh>>	m_Meshes;

	uint32_t						m_iNumMaterials;
	std::vector<SPtr<CResModelMaterial>>	m_Materials;

	uint32_t						m_iNumBones;
	std::vector<SPtr<CResModelBone>>		m_Bones;
	

	_bool							m_isAnimLoop = { true };
	uint32_t						m_iCurrentAnimIndex = {};
	uint32_t						m_iNumAnimations = {};
	std::vector<SPtr<CResModelAnim>>	m_Animations;


private:
	SPtr<CResStructuredBuffer> m_pGPUBones;
	SPtr<CResStructuredBuffer> m_pGPUAnimations;
	SPtr<CResStructuredBuffer> m_pGPUChannels;
	SPtr<CResStructuredBuffer> m_pGPUKeyFrames;
	SPtr<CResStructuredBuffer> m_pGPUBoneChannelMap;
	SPtr<CResStructuredBuffer> m_pGPUSkinBones;
	SPtr<CResStructuredBuffer> m_pGPUMeshSkinRanges;
	std::vector<GPU_MESH_SKIN_RANGE> m_GPUMeshSkinRanges;
private:
	_float4x4				m_PreTransformMatrix = {};
private:
	uint32_t						 m_iMaxBoneDepth = 0;
public:
	static SPtr<CResModel> Create(const _string& sPath);

};

NS_END
