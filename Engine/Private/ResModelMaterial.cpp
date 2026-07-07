#include "pch.h"
#include "ResModelMaterial.h"
#include "ResTexture2D.h"
#include <fstream>

NS_USING(Engine)

CResModelMaterial::CResModelMaterial(const _string& sPath)
	: CResource{ sPath }
{
}

CResModelMaterial::~CResModelMaterial()
{
}

HRESULT CResModelMaterial::Load(const std::any& arg)
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
	auto pPoint = descArg->ptr;
	{
		_char	szDrive[MAX_PATH] = { };
		_char	szDir[MAX_PATH] = { };

		_splitpath_s(strModelFilePath.c_str(), szDrive, MAX_PATH, szDir, MAX_PATH, nullptr, 0, nullptr, 0);

		m_iMaterialTypeNum = *(uint32_t*)pPoint;
		pPoint += sizeof(uint32_t);


		uint32_t textureTypeCount = *(uint32_t*)pPoint;
		pPoint += sizeof(uint32_t);

		

		for (size_t i = 0; i < textureTypeCount; i++)
		{
			uint32_t textureCount = *(uint32_t*)pPoint;
			pPoint += sizeof(uint32_t);

			
			for (size_t j = 0; j < textureCount; j++)
			{
				_char	szFileName[MAX_PATH] = { };
				_char	szExt[MAX_PATH] = { };

				uint32_t m_textureType = *(uint32_t*)pPoint;
				pPoint += sizeof(uint32_t);




				uint32_t len = *(uint32_t*)pPoint;
				pPoint += sizeof(uint32_t);


				std::string file;
				file.resize(len);
				memcpy(file.data(), pPoint, len);
				pPoint += len;


				uint32_t extLen = *(uint32_t*)pPoint;
				pPoint += sizeof(uint32_t);


				std::string ext;
				ext.resize(extLen);
				memcpy(ext.data(), pPoint, extLen);
				pPoint += extLen;


				_char	szFullPath[MAX_PATH] = {};

				strcpy_s(szFullPath, szDrive);
				strcat_s(szFullPath, szDir);
				strcat_s(szFullPath, file.c_str());
				strcat_s(szFullPath, ext.c_str());
				//strcat_s(szFullPath,".dds");
				std::filesystem::path fsPath(szFullPath);
				std::string sExt = fsPath.extension().string();
				std::filesystem::path ddsFsPath = fsPath;
				ddsFsPath.replace_extension(".dds");

				// dds가 실제로 존재하면 dds 사용
				if (std::filesystem::exists(ddsFsPath))
				{
					strcpy_s(szFullPath, MAX_PATH, ddsFsPath.string().c_str());
				}

				auto resTex = CResTexture2D::Create(szFullPath);
				if (FAILED(resTex->Load())) {
					return E_FAIL;
				}

				m_Materials[m_textureType].push_back(resTex);
			}
		}



	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResModelMaterial::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}




SPtr<CResModelMaterial> CResModelMaterial::Create(const _string& sPath)
{
	return ToSPtr(new CResModelMaterial{ sPath });
}
