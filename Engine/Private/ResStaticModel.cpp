#include "pch.h"
#include "ResStaticModel.h"
#include "ResStaticModelMesh.h"
#include "ResModelMaterial.h"
#include <fstream>

NS_USING(Engine)

CResStaticModel::CResStaticModel(const _string& sPath)
	: CResource{ sPath }
{
}

CResStaticModel::~CResStaticModel()
{
}

HRESULT CResStaticModel::Load(const std::any& arg)
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

		m_iNumMeshes = fh->MeshCount;
		m_iNumMaterials = fh->MaterialCount;

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
		};


	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResStaticModel::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

HRESULT CResStaticModel::Ready_Meshes(_char* ptr)
{

	for (size_t i = 0; i < m_iNumMeshes; i++)
	{

		uint32_t consumed = *(uint32_t*)ptr;
		ptr += sizeof(uint32_t);


		auto    pMesh = CResStaticModelMesh::Create();
		if (nullptr == pMesh)
			return E_FAIL;

		E::CResStaticModelMesh::DESC pDesc{};
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


HRESULT CResStaticModel::Ready_Materials(const _string& strModelFilePath, _char* ptr)
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


SPtr<CResStaticModel> CResStaticModel::Create(const _string& sPath)
{
	return ToSPtr(new CResStaticModel{ sPath });
}
