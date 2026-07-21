#pragma once

#include "Engine_Defines.h"

#include <string>
#include <utility>

NS_BEGIN(Engine)

enum class SERIALIZE_ERROR : uint8_t
{
	NONE,
	INVALID_PATH,
	SERIALIZER_CREATION_FAILED,
	DESERIALIZER_CREATION_FAILED,
	TARGET_PREPARATION_FAILED,
	DATA_SERIALIZATION_FAILED,
	TEMP_FILE_WRITE_FAILED,
	FILE_COMMIT_FAILED,
	SOURCE_FILE_INVALID,
	DATA_DESERIALIZATION_FAILED,
	TRAILING_DATA
};

inline const char* GetSerializeErrorName(SERIALIZE_ERROR error) noexcept
{
	switch (error)
	{
	case SERIALIZE_ERROR::NONE: return "NONE";
	case SERIALIZE_ERROR::INVALID_PATH: return "INVALID_PATH";
	case SERIALIZE_ERROR::SERIALIZER_CREATION_FAILED: return "SERIALIZER_CREATION_FAILED";
	case SERIALIZE_ERROR::DESERIALIZER_CREATION_FAILED: return "DESERIALIZER_CREATION_FAILED";
	case SERIALIZE_ERROR::TARGET_PREPARATION_FAILED: return "TARGET_PREPARATION_FAILED";
	case SERIALIZE_ERROR::DATA_SERIALIZATION_FAILED: return "DATA_SERIALIZATION_FAILED";
	case SERIALIZE_ERROR::TEMP_FILE_WRITE_FAILED: return "TEMP_FILE_WRITE_FAILED";
	case SERIALIZE_ERROR::FILE_COMMIT_FAILED: return "FILE_COMMIT_FAILED";
	case SERIALIZE_ERROR::SOURCE_FILE_INVALID: return "SOURCE_FILE_INVALID";
	case SERIALIZE_ERROR::DATA_DESERIALIZATION_FAILED: return "DATA_DESERIALIZATION_FAILED";
	case SERIALIZE_ERROR::TRAILING_DATA: return "TRAILING_DATA";
	default: return "UNKNOWN";
	}
}

struct SERIALIZE_RESULT
{
	HRESULT hResult{ S_OK };
	SERIALIZE_ERROR eError{ SERIALIZE_ERROR::NONE };
	std::string sPath{};
	std::string sMessage{};

	[[nodiscard]] bool Succeeded() const noexcept { return SUCCEEDED(hResult); }
	[[nodiscard]] bool Failed() const noexcept { return FAILED(hResult); }

	static SERIALIZE_RESULT Success(std::string path = {})
	{
		return { S_OK, SERIALIZE_ERROR::NONE, std::move(path), {} };
	}

	static SERIALIZE_RESULT Failure(
		SERIALIZE_ERROR error,
		HRESULT result,
		std::string path,
		std::string message)
	{
		return { result, error, std::move(path), std::move(message) };
	}
};

NS_END
