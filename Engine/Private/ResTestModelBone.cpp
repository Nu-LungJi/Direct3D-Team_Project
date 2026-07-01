#include "pch.h"
#include "ResTestModelBone.h"

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

CResTestModelBone::CResTestModelBone(const _string& sPath)
	: CResource{ sPath }
{
}

CResTestModelBone::~CResTestModelBone()
{
}

HRESULT CResTestModelBone::Load(const std::any& arg)
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
	auto& pAINode = descArg->pAINode;
	auto iParentIndex = descArg->iParentIndex;
	{
		strcpy_s(m_szName, pAINode->mName.C_Str());

		memcpy(&m_TransformationMatrix, &pAINode->mTransformation, sizeof(_float4x4));

		XMStoreFloat4x4(&m_TransformationMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_TransformationMatrix)));
		XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMMatrixIdentity());

		m_iParentBoneIndex = iParentIndex;
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResTestModelBone::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

void CResTestModelBone::Update_CombinedTransformationMatrix(const std::vector<SPtr<CResTestModelBone>>& Bones, _fmatrix PreTransformMatrix)
{
	if (-1 == m_iParentBoneIndex) {
		XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMLoadFloat4x4(&m_TransformationMatrix) * PreTransformMatrix);
		return;
	}


	XMStoreFloat4x4(&m_CombinedTransformationMatrix,
		XMLoadFloat4x4(&m_TransformationMatrix) * XMLoadFloat4x4(&Bones[m_iParentBoneIndex]->m_CombinedTransformationMatrix));
}


SPtr<CResTestModelBone> CResTestModelBone::Create(const _string& sPath)
{
	return ToSPtr(new CResTestModelBone{ sPath });
}
