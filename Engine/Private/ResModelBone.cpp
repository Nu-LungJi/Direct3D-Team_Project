#include "pch.h"
#include "ResModelBone.h"


#include <fstream>

NS_USING(Engine)

CResModelBone::CResModelBone(const _string& sPath)
	: CResource{ sPath }
{
}

CResModelBone::~CResModelBone()
{
}

HRESULT CResModelBone::Load(const std::any& arg)
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

	auto pPoint = descArg->ptr;
	{
		uint32_t nameLen;
		memcpy(&nameLen, pPoint, sizeof(uint32_t));
		pPoint += sizeof(uint32_t);


		std::string name;
		name.resize(nameLen);

		memcpy(name.data(), pPoint, nameLen);
		pPoint += nameLen;

		strcpy_s(m_szName, name.c_str());


		memcpy(&m_TransformationMatrix,pPoint,sizeof(_float4x4));
		pPoint += sizeof(_float4x4);

		memcpy(&m_iParentBoneIndex,pPoint,sizeof(uint32_t));
		pPoint += sizeof(uint32_t);

		XMStoreFloat4x4(&m_TransformationMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_TransformationMatrix)));
		XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMMatrixIdentity());

	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResModelBone::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

void CResModelBone::Update_CombinedTransformationMatrix(const std::vector<SPtr<CResModelBone>>& Bones, _fmatrix PreTransformMatrix)
{
	if (-1 == m_iParentBoneIndex) {
		XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMLoadFloat4x4(&m_TransformationMatrix) * PreTransformMatrix);
		return;
	}


	XMStoreFloat4x4(&m_CombinedTransformationMatrix,
		XMLoadFloat4x4(&m_TransformationMatrix) * XMLoadFloat4x4(&Bones[m_iParentBoneIndex]->m_CombinedTransformationMatrix));
}



SPtr<CResModelBone> CResModelBone::Create(const _string& sPath)
{
	return ToSPtr(new CResModelBone{ sPath });
}
