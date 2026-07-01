
#pragma once

#include "Resource.h"
#include "ResTestModelBone.h"
#include "ResTestModelMesh.h"
#include "ResTestModelMaterial.h"
#include "ResTestModelAnim.h"
// Assimp 전방선언
struct aiScene;

namespace Assimp
{
	class Importer;
}


NS_BEGIN(Engine)

class ENGINE_DLL CResTestModel final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResTestModel, CResource)
public:
	typedef struct tagDesc {
		MODEL eModelType; _matrix PreTransformMatrix;
	}DESC;
private:
	explicit CResTestModel(const _string& sPath);
	~CResTestModel() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;


private:
	HRESULT Ready_Bones(const aiNode* pAINode, int32_t iParentBoneIndex);
	HRESULT Ready_Materials(const _string& strModelFilePath);
	HRESULT Ready_Meshes();
	HRESULT Ready_Animation();
public:
	uint32_t Get_NumMeshes() const {
		return m_iNumMeshes;
	}

	int32_t Get_BoneIndex(const _char* pBoneName);

	const _float4x4* Get_BoneMatrixPtr(const _char* pBoneName);

	_bool Play_Animation(_float fTimeDelta);

public:
	HRESULT Bind_BoneMatrices( uint32_t iMeshIndex);
	HRESULT Bind_Materials( uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex);


public:
	std::vector<SPtr<CResTestModelMesh>>& GetMeshes() { return m_Meshes; }
	std::vector<SPtr<CResTestModelMaterial>>& GetMaterials() { return m_Materials; }
	std::vector<SPtr<CResTestModelAnim>>& GetAnimations() { return m_Animations; }


protected:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

private:
	MODEL						m_eModelType = {};
	uint32_t					m_iNumMeshes = {};
	std::vector<SPtr<CResTestModelMesh>>	m_Meshes;

	uint32_t						m_iNumMaterials;
	std::vector<SPtr<CResTestModelMaterial>>	m_Materials;

	std::vector<SPtr<CResTestModelBone>>		m_Bones;


	_bool							m_isAnimLoop = { true };
	uint32_t						m_iCurrentAnimIndex = {};
	uint32_t						m_iNumAnimations = {};
	std::vector<SPtr<CResTestModelAnim>>	m_Animations;

private:
	const aiScene* m_pAIScene = { nullptr };
	std::shared_ptr<Assimp::Importer>		m_Importer = {};
	_float4x4				m_PreTransformMatrix = {};

public:
	static SPtr<CResTestModel> Create(const _string& sPath);
};

NS_END