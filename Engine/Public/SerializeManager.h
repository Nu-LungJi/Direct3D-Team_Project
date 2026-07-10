#pragma once
#include "Engine_Defines.h"

#include "BinSerializer.h"
#include "BinDeSerializer.h"
#include "JsonSerializer.h"
#include "JsonDeSerializer.h"

#include <filesystem> 
#include <exception>  

NS_BEGIN(Engine)

class CSerializeManager final : public CEngineBase
{
private:
	CSerializeManager();
	~CSerializeManager();

public:
	void UpdateGUI();

public:
	template<typename T>
	HRESULT JsonSerialize(const std::string& path, const T& value, const std::string& rootName = "JSON")
	{
		auto pJsonSer = CJsonSerializer::Create();
		if (!pJsonSer) {
			MSG_BOX_STR(StringToWString({ "SerializeJson Create: " + path }).c_str());
			return E_FAIL;
		}

		// 1. 임시 파일 경로 생성
		std::string tempPath = path + ".tmp";

		try
		{
			pJsonSer->Write(rootName, value);

			// 2. 임시 파일(.tmp)에 먼저 저장
			if (FAILED(pJsonSer->SaveToFile(tempPath)))
			{
				return E_FAIL;
			}

			// 3. 쓰기가 완벽히 끝났으므로 원본 파일과 교체 (원자적 교체)
			std::error_code ec;
			if (std::filesystem::exists(path)) {
				std::filesystem::remove(path, ec); // 기존 원본 삭제
			}
			std::filesystem::rename(tempPath, path, ec); // tmp를 원본 이름으로 변경

			if (ec) return E_FAIL; // 이름 변경 실패 시
		}
		catch (const std::exception& e)
		{
			// 쓰는 도중 에러가 나면 쓰레기 파일(.tmp) 삭제
			std::error_code ec;
			std::filesystem::remove(tempPath, ec);

			MSG_BOX_STR(StringToWString({ std::string("Json Save Error: ") + e.what() }).c_str());
			return E_FAIL;
		}

		return S_OK;
	}

	template<typename T>
	HRESULT JsonDeSerialize(const std::string& path, T& outValue, const std::string& rootName = "JSON")
	{
		auto pDese = CJsonDeSerializer::Create(path);
		if (!pDese)
		{
			MSG_BOX_STR(StringToWString({ "DeSerializeJson Create: " + path }).c_str());
			return E_FAIL;
		}

		//  파일 오염, 타입 불일치 등으로 인한 크래시 방어막
		try
		{
			pDese->Read(rootName, outValue);
		}
		catch (const std::exception& e)
		{
			std::string errMsg = "Json Load Failed!\nFile: " + path + "\nReason: " + e.what();
			MSG_BOX_STR(StringToWString(errMsg).c_str());
			return E_FAIL;
		}

		return S_OK;
	}

	template<typename T>
	HRESULT BinSerialize(const std::string& path, const T& value, const std::string& rootName = "BIN")
	{
		auto pBinSer = CBinSerializer::Create();
		if (!pBinSer) {
			MSG_BOX_STR(StringToWString({ "BinSerialize Create: " + path }).c_str());
			return E_FAIL;
		}

		std::string tempPath = path + ".tmp";

		try
		{
			pBinSer->Write(rootName, value);
			if (FAILED(pBinSer->SaveToFile(tempPath))) return E_FAIL;

			std::error_code ec;
			if (std::filesystem::exists(path)) std::filesystem::remove(path, ec);
			std::filesystem::rename(tempPath, path, ec);
			if (ec) return E_FAIL;
		}
		catch (const std::exception& e)
		{
			std::error_code ec;
			std::filesystem::remove(tempPath, ec);
			MSG_BOX_STR(StringToWString({ std::string("Bin Save Error: ") + e.what() }).c_str());
			return E_FAIL;
		}

		return S_OK;
	}

	template<typename T>
	HRESULT BinDeSerialize(const std::string& path, T& outValue, const std::string& rootName = "BIN")
	{
		auto pDese = CBinDeSerializer::Create(path);
		if (!pDese)
		{
			MSG_BOX_STR(StringToWString({ "BinDeSerialize Create: " + path }).c_str());
			return E_FAIL;
		}

		try
		{
			pDese->Read(rootName, outValue);
		}
		catch (const std::exception& e)
		{
			std::string errMsg = "Binary Load Failed!\nFile: " + path + "\nReason: " + e.what();
			MSG_BOX_STR(StringToWString(errMsg).c_str());
			return E_FAIL;
		}

		return S_OK;
	}

private:
	HRESULT Initialize();

public:
	static UPtr<CSerializeManager> Create();
};

NS_END
