#pragma once
#include "Engine_Defines.h"

#include "BinSerializer.h"
#include "BinDeSerializer.h"
#include "JsonSerializer.h"
#include "JsonDeSerializer.h"

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

		pJsonSer->Write(rootName, value);
		if (FAILED(pJsonSer->SaveToFile(path)))
		{
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
		pDese->Read(rootName, outValue);

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

		pBinSer->Write(rootName, value);
		if (FAILED(pBinSer->SaveToFile(path)))
		{
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
		pDese->Read(rootName, outValue);

		return S_OK;
	}
private:
	HRESULT Initialize();

public:
	static UPtr<CSerializeManager> Create();
};

NS_END
