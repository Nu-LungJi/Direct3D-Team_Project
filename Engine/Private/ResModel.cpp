#include "pch.h"
#include "ResModel.h"
#include "ResModelMesh.h"
#include "ResTestModelBone.h"
#include "ResModelMaterial.h"
#include "ResTestModelAnim.h"
#include <fstream>

NS_USING(Engine)

CResModel::CResModel(const _string& sPath)
	: CResource{ sPath }
{
}

CResModel::~CResModel()
{
}

HRESULT CResModel::Load(const std::any& arg)
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

	m_eState = STATE::LOADING;
	XMStoreFloat4x4(&m_PreTransformMatrix, descArg->PreTransformMatrix);
	{	
		std::ifstream file(m_sPath, std::ios::binary | std::ios::ate);


		if (!file.is_open())
		{
			return E_FAIL;
		}

		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::shared_ptr<char[]> buffer = std::make_shared<char[]>(size);
		file.read(buffer.get(), size);

		file.close();

		char* ptr = buffer.get();

		MODEL_FILE_HEADER* fh = (MODEL_FILE_HEADER*)ptr;
		ptr += sizeof(MODEL_FILE_HEADER);
		
		m_iNumMeshes	= fh->MeshCount;
		m_iAnimCnt		= fh->AnimationCount;
		m_iNumMaterials = fh->MaterialCount;
		m_iBoneCnt		= fh->BoneCount;

		if (!fh->bHasBone && !fh->bHasAnimation)
			m_eModelType = MODEL::STATIC;
		else if (fh->bHasBone && !fh->bHasAnimation)
			m_eModelType = MODEL::SKELETAL;
	

		switch (m_eModelType) {
		case MODEL::STATIC:
		{
			char* base = buffer.get();
			char* end = base + size;

			while (ptr < end)
			{
				CHUCKHEADER* chunk = (CHUCKHEADER*)ptr;
				ptr += sizeof(CHUCKHEADER);

				switch (chunk->type)
				{
				case CHUNCK_TYPE::CHUNK_MESH:
				{
					char* meshData = ptr;

					if (FAILED(Ready_Meshes(meshData)))
						return E_FAIL;

					ptr += chunk->size;
				}
				break;

				case CHUNCK_TYPE::CHUNK_MATERIAL:
				{
					char* matData = ptr;

					if (FAILED(Ready_Materials(m_sPath, matData)))
						return E_FAIL;

					ptr += chunk->size;
				}
				break;
				}
			}
		;

		
		}
			break;

		case MODEL::SKELETAL:
		{

		/*	if (FAILED(Ready_Bones(m_pAIScene->mRootNode, -1)))
				return E_FAIL;*/

		/*	if (FAILED(Ready_Meshes()))
				return E_FAIL;

			if (FAILED(Ready_Materials(m_sPath)))
				return E_FAIL;

			if (FAILED(Ready_Animation()))
				return E_FAIL;*/

		}
		break;

		default:

			break;
		}

	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResModel::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

HRESULT CResModel::Ready_Bones(const aiNode* pAINode, int32_t iParentBoneIndex)
{
	/*auto    pBone = CResTestModelBone::Create();
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
	}*/

	return S_OK;
}

HRESULT CResModel::Ready_Materials(const _string& strModelFilePath, _char* ptr)
{
	m_Materials.resize(m_iNumMaterials);
	for (size_t i = 0; i < m_iNumMaterials; i++)
	{

		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);

		auto  pMaterial = CResModelMaterial::Create(strModelFilePath);

		if (FAILED(pMaterial->Load(CResModelMaterial::DESC{ .ptr = ptr }))) {
			return E_FAIL;
		}

		m_Materials[pMaterial->GetMaterialTypeNum()] = (pMaterial);


		ptr += consumed;
	}

	return S_OK;
}

HRESULT CResModel::Ready_Meshes(_char* ptr)
{

	for (size_t i = 0; i < m_iNumMeshes; i++)
	{
	
		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);


		auto    pMesh = CResModelMesh::Create();
		if (nullptr == pMesh)
			return E_FAIL;

		E::CResModelMesh::DESC pDesc{};
		pDesc.eType = m_eModelType;
		pDesc.ptr = ptr;
		pDesc.pModel = this;
		pDesc.PreTransformMatrix = XMLoadFloat4x4(&m_PreTransformMatrix);
	
		if (FAILED(pMesh->Load(pDesc))) {
			return E_FAIL;
		}

		m_Meshes.push_back(pMesh);

		ptr += consumed;
	}

	return S_OK;
}



HRESULT CResModel::Ready_Animation()
{
	//m_iNumAnimations = m_pAIScene->mNumAnimations;

	//for (size_t i = 0; i < m_iNumAnimations; i++)
	//{
	//	auto pAnimation = CResTestModelAnim::Create();
	//	if (nullptr == pAnimation)
	//		return E_FAIL;

	//	if (FAILED(pAnimation->Load(CResTestModelAnim::DESC{ .pAIAnimation = m_pAIScene->mAnimations[i] ,.pModel = this })))
	//	{
	//		return E_FAIL;
	//	}
	//	m_Animations.push_back(pAnimation);
	//}

	return S_OK;
}

int32_t CResModel::Get_BoneIndex(const _char* pBoneName)
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

const _float4x4* CResModel::Get_BoneMatrixPtr(const _char* pBoneName)
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

_bool  CResModel::Play_Animation(_float fTimeDelta)
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

SPtr<CResModel> CResModel::Create(const _string& sPath)
{
	return ToSPtr(new CResModel{ sPath });
}
