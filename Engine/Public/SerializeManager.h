#pragma once
#include "Engine_Defines.h"

#include "BinSerializer.h"
#include "BinDeSerializer.h"
#include "JsonSerializer.h"
#include "JsonDeSerializer.h"

#include <filesystem> 
#include <exception>  
#include <stdexcept>
#include <type_traits>
#include <utility>

NS_BEGIN(Engine)

class ENGINE_DLL CSerializeManager final : public CEngineBase
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

		const std::filesystem::path targetPath{ path };
		std::filesystem::path tempPath;

		try
		{
			if (FAILED(PrepareSaveTarget(targetPath))) return E_FAIL;
			tempPath = MakeTemporaryPath(targetPath);

			pJsonSer->Write(rootName, value);

			if (FAILED(pJsonSer->SaveToFile(tempPath.string())))
			{
				RemoveTemporaryFile(tempPath);
				return E_FAIL;
			}

			if (FAILED(CommitTemporaryFile(tempPath, targetPath)))
			{
				RemoveTemporaryFile(tempPath);
				return E_FAIL;
			}
		}
		catch (const std::exception& e)
		{
			RemoveTemporaryFile(tempPath);

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
			DeserializeToTemporary(outValue, [&](T& loadedValue)
			{
				pDese->Read(rootName, loadedValue);
			});
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

		const std::filesystem::path targetPath{ path };
		std::filesystem::path tempPath;

		try
		{
			if (FAILED(PrepareSaveTarget(targetPath))) return E_FAIL;
			tempPath = MakeTemporaryPath(targetPath);

			pBinSer->Write(rootName, value);
			if (FAILED(pBinSer->SaveToFile(tempPath.string())))
			{
				RemoveTemporaryFile(tempPath);
				return E_FAIL;
			}

			if (FAILED(CommitTemporaryFile(tempPath, targetPath)))
			{
				RemoveTemporaryFile(tempPath);
				return E_FAIL;
			}
		}
		catch (const std::exception& e)
		{
			RemoveTemporaryFile(tempPath);
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
			DeserializeToTemporary(outValue, [&](T& loadedValue)
			{
				pDese->Read(rootName, loadedValue);
				if (!pDese->IsFullyConsumed())
					throw std::runtime_error("Binary payload contains unread trailing data");
			});
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
	template<typename T, typename Loader>
	static void DeserializeToTemporary(T& outValue, Loader&& loader)
	{
		constexpr bool bCanAssign =
			std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>;
		static_assert(bCanAssign,
			"Safe deserialization requires a move-assignable or copy-assignable value type");
		static_assert(
			std::is_copy_constructible_v<T> || std::is_default_constructible_v<T>,
			"Safe deserialization requires a copy-constructible or default-constructible value type");

		if constexpr (std::is_copy_constructible_v<T>)
		{
			T loadedValue{ outValue };
			std::forward<Loader>(loader)(loadedValue);
			AssignLoadedValue(outValue, loadedValue);
		}
		else
		{
			T loadedValue{};
			std::forward<Loader>(loader)(loadedValue);
			AssignLoadedValue(outValue, loadedValue);
		}
	}

	template<typename T>
	static void AssignLoadedValue(T& outValue, T& loadedValue)
	{
		if constexpr (std::is_move_assignable_v<T>)
			outValue = std::move(loadedValue);
		else
			outValue = loadedValue;
	}

	HRESULT Initialize();
	HRESULT PrepareSaveTarget(const std::filesystem::path& targetPath) const;
	std::filesystem::path MakeTemporaryPath(const std::filesystem::path& targetPath) const;
	HRESULT CommitTemporaryFile(
		const std::filesystem::path& tempPath,
		const std::filesystem::path& targetPath) const;
	void RemoveTemporaryFile(const std::filesystem::path& tempPath) const noexcept;

public:
	static UPtr<CSerializeManager> Create();
};

NS_END
