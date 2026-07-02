#include "pch.h"
#include "ResTestModelMaterial.h"
#include "ResTexture2D.h"
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

CResTestModelMaterial::CResTestModelMaterial(const _string& sPath)
	: CResource{ sPath }
{
}

CResTestModelMaterial::~CResTestModelMaterial()
{
}

HRESULT CResTestModelMaterial::Load(const std::any& arg)
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
	auto& strModelFilePath = m_sPath;
	auto& pAIMaterial = descArg->pAIMaterial;

	{
		_char	szDrive[MAX_PATH] = { };
		_char	szDir[MAX_PATH] = { };

		_splitpath_s(strModelFilePath.c_str(), szDrive, MAX_PATH, szDir, MAX_PATH, nullptr, 0, nullptr, 0);


		for (size_t i = 0; i < AI_TEXTURE_TYPE_MAX; i++)
		{

			uint32_t		iNumTextures = pAIMaterial->GetTextureCount(static_cast<aiTextureType>(i));

			for (size_t j = 0; j < iNumTextures; j++)
			{
				_char	szFileName[MAX_PATH] = { };
				_char	szExt[MAX_PATH] = { };

				aiString		strTexturePath = {};

				if (FAILED(pAIMaterial->GetTexture(static_cast<aiTextureType>(i), j, &strTexturePath)))
					return E_FAIL;

				_splitpath_s(strTexturePath.C_Str(), nullptr, 0, nullptr, 0, szFileName, MAX_PATH, szExt, MAX_PATH);

				_char	szFullPath[MAX_PATH] = {};

				strcpy_s(szFullPath, szDrive);
				strcat_s(szFullPath, szDir);
				strcat_s(szFullPath, szFileName);
				strcat_s(szFullPath, szExt);

				auto resTex = CResTexture2D::Create(szFullPath);
				if (FAILED(resTex->Load())) {
					return E_FAIL;
				}

			/*	HRESULT         hr = {};
				ComPtr<ID3D11ShaderResourceView>		pSRV = { nullptr };

				_tchar	szFinalPath[MAX_PATH] = {};

				MultiByteToWideChar(CP_ACP, 0, szFullPath, strlen(szFullPath),
					szFinalPath, MAX_PATH);



				if (false == strcmp(szExt, ".dds"))
					hr = CreateDDSTextureFromFile(m_pDevice.Get(), szFinalPath, nullptr, &pSRV);

				else if (false == strcmp(szExt, ".tga"))
					hr = E_FAIL;
				else
					hr = CreateWICTextureFromFile(m_pDevice.Get(), szFinalPath, nullptr, &pSRV);*/

				m_Materials[i].push_back(resTex);
			}
		}




	}
	
	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResTestModelMaterial::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}



SPtr<CResTestModelMaterial> CResTestModelMaterial::Create(const _string& sPath)
{
	return ToSPtr(new CResTestModelMaterial{ sPath });
}
