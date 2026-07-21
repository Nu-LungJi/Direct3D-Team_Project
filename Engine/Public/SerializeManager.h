#pragma once

#include "Engine_Defines.h"

#include "BinSerializer.h"
#include "BinDeSerializer.h"
#include "JsonSerializer.h"
#include "JsonDeSerializer.h"
#include "SerializeResult.h"

#include <exception>
#include <filesystem>
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
	HRESULT JsonSerialize(
		const std::string& path,
		const T& value,
		const std::string& rootName = "JSON",
		bool bShowError = true)
	{
		const SERIALIZE_RESULT result = JsonSerializeDetailed(path, value, rootName);
		if (bShowError && result.Failed()) ShowDetailedError("JSON Save", result);
		return result.hResult;
	}

	template<typename T>
	SERIALIZE_RESULT JsonSerializeDetailed(
		const std::string& path,
		const T& value,
		const std::string& rootName = "JSON")
	{
		if (path.empty())
			return MakeFailure(
				SERIALIZE_ERROR::INVALID_PATH, E_INVALIDARG, path, "Save path is empty");

		auto serializer = CJsonSerializer::Create();
		if (!serializer)
			return MakeFailure(
				SERIALIZE_ERROR::SERIALIZER_CREATION_FAILED,
				E_FAIL,
				path,
				"Failed to create the JSON serializer");

		std::filesystem::path targetPath{};
		std::filesystem::path tempPath{};
		try
		{
			targetPath = std::filesystem::path{ path };
			const HRESULT prepareResult = PrepareSaveTarget(targetPath);
			if (FAILED(prepareResult))
				return MakeFailure(
					SERIALIZE_ERROR::TARGET_PREPARATION_FAILED,
					prepareResult,
					path,
					"Failed to prepare the save directory");

			tempPath = MakeTemporaryPath(targetPath);
			serializer->Write(rootName, value);

			if (FAILED(serializer->SaveToFile(tempPath.string())))
			{
				RemoveTemporaryFile(tempPath);
				return MakeFailure(
					SERIALIZE_ERROR::TEMP_FILE_WRITE_FAILED,
					E_FAIL,
					path,
					"Failed to write the temporary JSON file");
			}

			const HRESULT commitResult = CommitTemporaryFile(tempPath, targetPath);
			if (FAILED(commitResult))
			{
				RemoveTemporaryFile(tempPath);
				return MakeFailure(
					SERIALIZE_ERROR::FILE_COMMIT_FAILED,
					commitResult,
					path,
					"Failed to replace the destination file");
			}
		}
		catch (const std::exception& e)
		{
			RemoveTemporaryFile(tempPath);
			return MakeFailure(
				SERIALIZE_ERROR::DATA_SERIALIZATION_FAILED, E_FAIL, path, e.what());
		}

		return SERIALIZE_RESULT::Success(path);
	}

	template<typename T>
	HRESULT JsonDeSerialize(
		const std::string& path,
		T& outValue,
		const std::string& rootName = "JSON",
		bool bShowError = true)
	{
		const SERIALIZE_RESULT result = JsonDeSerializeDetailed(path, outValue, rootName);
		if (bShowError && result.Failed()) ShowDetailedError("JSON Load", result);
		return result.hResult;
	}

	template<typename T>
	SERIALIZE_RESULT JsonDeSerializeDetailed(
		const std::string& path,
		T& outValue,
		const std::string& rootName = "JSON")
	{
		if (path.empty())
			return MakeFailure(
				SERIALIZE_ERROR::INVALID_PATH, E_INVALIDARG, path, "Load path is empty");

		auto deserializer = CJsonDeSerializer::Create(path);
		if (!deserializer)
			return MakeFailure(
				SERIALIZE_ERROR::SOURCE_FILE_INVALID,
				E_FAIL,
				path,
				"The JSON file could not be opened or parsed");

		try
		{
			DeserializeToTemporary(outValue, [&](T& loadedValue)
			{
				deserializer->Read(rootName, loadedValue);
			});
		}
		catch (const std::exception& e)
		{
			return MakeFailure(
				SERIALIZE_ERROR::DATA_DESERIALIZATION_FAILED, E_FAIL, path, e.what());
		}

		return SERIALIZE_RESULT::Success(path);
	}

	template<typename T>
	HRESULT BinSerialize(
		const std::string& path,
		const T& value,
		const std::string& rootName = "BIN",
		bool bShowError = true)
	{
		const SERIALIZE_RESULT result = BinSerializeDetailed(path, value, rootName);
		if (bShowError && result.Failed()) ShowDetailedError("Binary Save", result);
		return result.hResult;
	}

	template<typename T>
	SERIALIZE_RESULT BinSerializeDetailed(
		const std::string& path,
		const T& value,
		const std::string& rootName = "BIN")
	{
		if (path.empty())
			return MakeFailure(
				SERIALIZE_ERROR::INVALID_PATH, E_INVALIDARG, path, "Save path is empty");

		auto serializer = CBinSerializer::Create();
		if (!serializer)
			return MakeFailure(
				SERIALIZE_ERROR::SERIALIZER_CREATION_FAILED,
				E_FAIL,
				path,
				"Failed to create the Binary serializer");

		std::filesystem::path targetPath{};
		std::filesystem::path tempPath{};
		try
		{
			targetPath = std::filesystem::path{ path };
			const HRESULT prepareResult = PrepareSaveTarget(targetPath);
			if (FAILED(prepareResult))
				return MakeFailure(
					SERIALIZE_ERROR::TARGET_PREPARATION_FAILED,
					prepareResult,
					path,
					"Failed to prepare the save directory");

			tempPath = MakeTemporaryPath(targetPath);
			serializer->Write(rootName, value);

			if (FAILED(serializer->SaveToFile(tempPath.string())))
			{
				RemoveTemporaryFile(tempPath);
				return MakeFailure(
					SERIALIZE_ERROR::TEMP_FILE_WRITE_FAILED,
					E_FAIL,
					path,
					"Failed to write the temporary Binary file");
			}

			const HRESULT commitResult = CommitTemporaryFile(tempPath, targetPath);
			if (FAILED(commitResult))
			{
				RemoveTemporaryFile(tempPath);
				return MakeFailure(
					SERIALIZE_ERROR::FILE_COMMIT_FAILED,
					commitResult,
					path,
					"Failed to replace the destination file");
			}
		}
		catch (const std::exception& e)
		{
			RemoveTemporaryFile(tempPath);
			return MakeFailure(
				SERIALIZE_ERROR::DATA_SERIALIZATION_FAILED, E_FAIL, path, e.what());
		}

		return SERIALIZE_RESULT::Success(path);
	}

	template<typename T>
	HRESULT BinDeSerialize(
		const std::string& path,
		T& outValue,
		const std::string& rootName = "BIN",
		bool bShowError = true)
	{
		const SERIALIZE_RESULT result = BinDeSerializeDetailed(path, outValue, rootName);
		if (bShowError && result.Failed()) ShowDetailedError("Binary Load", result);
		return result.hResult;
	}

	template<typename T>
	SERIALIZE_RESULT BinDeSerializeDetailed(
		const std::string& path,
		T& outValue,
		const std::string& rootName = "BIN")
	{
		if (path.empty())
			return MakeFailure(
				SERIALIZE_ERROR::INVALID_PATH, E_INVALIDARG, path, "Load path is empty");

		auto deserializer = CBinDeSerializer::Create(path);
		if (!deserializer)
			return MakeFailure(
				SERIALIZE_ERROR::SOURCE_FILE_INVALID,
				E_FAIL,
				path,
				"The Binary file could not be opened or failed header validation");

		try
		{
			DeserializeToTemporary(outValue, [&](T& loadedValue)
			{
				deserializer->Read(rootName, loadedValue);
				if (!deserializer->IsFullyConsumed())
					throw CTrailingDataError{};
			});
		}
		catch (const CTrailingDataError& e)
		{
			return MakeFailure(SERIALIZE_ERROR::TRAILING_DATA, E_FAIL, path, e.what());
		}
		catch (const std::exception& e)
		{
			return MakeFailure(
				SERIALIZE_ERROR::DATA_DESERIALIZATION_FAILED, E_FAIL, path, e.what());
		}

		return SERIALIZE_RESULT::Success(path);
	}

private:
	class CTrailingDataError final : public std::runtime_error
	{
	public:
		CTrailingDataError()
			: std::runtime_error{ "Binary payload contains unread trailing data" }
		{
		}
	};

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
		if constexpr (std::is_move_assignable_v<T>) outValue = std::move(loadedValue);
		else outValue = loadedValue;
	}

	static SERIALIZE_RESULT MakeFailure(
		SERIALIZE_ERROR error,
		HRESULT result,
		const std::string& path,
		std::string message)
	{
		return SERIALIZE_RESULT::Failure(error, result, path, std::move(message));
	}

	static void ShowDetailedError(const char* operation, const SERIALIZE_RESULT& result)
	{
		std::string message = std::string{ operation } + " Failed\nCode: " +
			GetSerializeErrorName(result.eError) + "\nFile: " + result.sPath;
		if (!result.sMessage.empty()) message += "\nReason: " + result.sMessage;
		MSG_BOX_STR(StringToWString(message).c_str());
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
