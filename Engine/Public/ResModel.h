
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
	_float4x4				m_PreTransformMatrix = {};

public:
	static SPtr<CResModel> Create(const _string& sPath);
};

NS_END