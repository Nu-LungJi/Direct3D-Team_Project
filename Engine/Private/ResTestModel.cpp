#include "pch.h"
#include "ResTestModel.h"
#include "ResTestModelMesh.h"
#include "ResTestModelBone.h"
#include "ResTestModelMaterial.h"
#include "ResTestModelAnim.h"
#ifdef _DEBUG
#ifdef new
#pragma push_macro("new")
#undef new
#define RESTORE_NEW_MACRO
#endif
#endif

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#ifdef _DEBUG
#ifdef RESTORE_NEW_MACRO
#pragma pop_macro("new")
#undef RESTORE_NEW_MACRO
#endif
#endif

#include <fstream>

NS_USING(Engine)

CResTestModel::CResTestModel(const _string& sPath)
	: CResource{ sPath }
{
}

CResTestModel::~CResTestModel()
{
}

HRESULT CResTestModel::Load(const std::any& arg)
{
	auto descArg = std::any_cast<DESC>(&arg);
	if (!descArg)
	{
		return E_FAIL;
	}

	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	m_Importer = std::make_shared<Assimp::Importer>();

	m_eState = STATE::LOADING;
	m_eModelType = descArg->eModelType;
	{

		uint32_t        iFlag = { aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast };
		if (MODEL::NONANIM == m_eModelType)

		if (MODEL::STATIC == m_eModelType)
			iFlag |= aiProcess_PreTransformVertices;

		iFlag |= aiProcess_PopulateArmatureData;							// 애니메이션 최적화(본-노드 사이의 연산 단순화)
		iFlag |= aiProcess_GlobalScale;										// Blender 편집 크기와 DirectX에서의 크기를 동기화
		iFlag |= aiProcess_OptimizeMeshes;									// 너무 잘게 쪼개진 메쉬 통합시켜 DrawCall 낮춤.
		iFlag |= aiProcess_ImproveCacheLocality;							// 캐시 히트율을 증가 시킴. (데이터 순서를 재배치)

		//GraphicsFlags |= aiProcessPreset_TargetRealtime_Quality;
		//GraphicsFlags |= aiProcessPreset_TargetRealtime_MaxQuality;

		m_Importer->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true);	// FBX 파일의 계층 구조를 원본 그대로 유지시킴.
		//m_Importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 0.25f);		// 모델을 import할 때, 배율을 지정.

		m_pAIScene = m_Importer->ReadFile(m_sPath.c_str(), iFlag);
		if (nullptr == m_pAIScene)
			return E_FAIL;

	
		XMStoreFloat4x4(&m_PreTransformMatrix, descArg->PreTransformMatrix);
		Ready_Bones(m_pAIScene->mRootNode, -1);

		if (FAILED(Ready_Meshes()))
			return E_FAIL;

		if (FAILED(Ready_Materials(m_sPath)))
			return E_FAIL;

		if (FAILED(Ready_Animation()))
			return E_FAIL;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResTestModel::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

HRESULT CResTestModel::Ready_Bones(const aiNode* pAINode, int32_t iParentBoneIndex)
{
	auto    pBone = CResTestModelBone::Create();
	if (nullptr == pBone) {
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}

	E::CResTestModelBone::DESC Desc{};
	Desc.pAINode = pAINode;
	Desc.iParentIndex = iParentBoneIndex;
	if (FAILED(pBone->Load(Desc))) {
		m_eState = STATE::LOADFAIL;
		return E_FAIL;
	}	

	m_Bones.push_back(pBone);

	int32_t iParentIndex = m_Bones.size() - 1;

	for (uint32_t i = 0; i < pAINode->mNumChildren; ++i)
	{
		Ready_Bones(pAINode->mChildren[i], iParentIndex);
	}




	return S_OK;
}

HRESULT CResTestModel::Ready_Materials(const _string& strModelFilePath)
{
	m_iNumMaterials = m_pAIScene->mNumMaterials;

	for (size_t i = 0; i < m_iNumMaterials; i++)
	{
		auto  pMaterial = CResTestModelMaterial::Create(strModelFilePath);

		if (FAILED(pMaterial->Load(CResTestModelMaterial::DESC{.pAIMaterial = m_pAIScene->mMaterials[i]}))) {
			return E_FAIL;
		}

		m_Materials.push_back(pMaterial);
	}

	return S_OK;
}

HRESULT CResTestModel::Ready_Meshes()
{
	m_iNumMeshes = m_pAIScene->mNumMeshes;

	for (size_t i = 0; i < m_iNumMeshes; i++)
	{

		auto    pAIMesh = CResTestModelMesh::Create();
		if (nullptr == pAIMesh)
			return E_FAIL;

		E::CResTestModelMesh::DESC pDesc{};
		pDesc.eType = m_eModelType;
		pDesc.pAIMesh = m_pAIScene->mMeshes[i];
		pDesc.pModel = this;
		pDesc.PreTransformMatrix = XMLoadFloat4x4(&m_PreTransformMatrix);
		if (FAILED(pAIMesh->Load(pDesc))) {
			return E_FAIL;
		}

		m_Meshes.push_back(pAIMesh);
	}

	return S_OK;
}

HRESULT CResTestModel::Ready_Animation()
{
	m_iNumAnimations = m_pAIScene->mNumAnimations;

	for (size_t i = 0; i < m_iNumAnimations; i++)
	{
		auto pAnimation = CResTestModelAnim::Create();
		if (nullptr == pAnimation)
			return E_FAIL;

		if (FAILED(pAnimation->Load(CResTestModelAnim::DESC{.pAIAnimation = m_pAIScene->mAnimations[i] ,.pModel = this})))
		{
			return E_FAIL;
		}
		m_Animations.push_back(pAnimation);
	}

	return S_OK;
}

int32_t CResTestModel::Get_BoneIndex(const _char* pBoneName)
{
	int32_t iBoneIndex = { 0 };
	auto    iter = find_if(m_Bones.begin(), m_Bones.end(), [&](SPtr<CResTestModelBone> pBone)->_bool
		{
			if (true == pBone->Compare_Name(pBoneName))
				return true;

			++iBoneIndex;

			return false;
		});

	if (iter == m_Bones.end())
		return -1;

	return iBoneIndex;
}

const _float4x4* CResTestModel::Get_BoneMatrixPtr(const _char* pBoneName)
{
	auto    iter = find_if(m_Bones.begin(), m_Bones.end(), [&](SPtr<CResTestModelBone> pBone)->_bool
		{
			if (true == pBone->Compare_Name(pBoneName))
				return true;

			return false;
		});

	if (iter == m_Bones.end())
		return nullptr;

	return (*iter)->Get_CombinedTransformationMatrixPtr();
}

_bool  CResTestModel::Play_Animation(_float fTimeDelta)
{
	_bool           isFinished = { false };

	/* 뼈들의 m_TransformationMatrix를 갱신해준다. */
	isFinished = m_Animations[m_iCurrentAnimIndex]->Update_TransformationMatrices(fTimeDelta, m_Bones, m_isAnimLoop);

	for (auto& pBone : m_Bones)
	{
		pBone->Update_CombinedTransformationMatrix(m_Bones, XMLoadFloat4x4(&m_PreTransformMatrix));
	}

	return isFinished;


}

SPtr<CResTestModel> CResTestModel::Create(const _string& sPath)
{
	return ToSPtr(new CResTestModel{ sPath });
}
