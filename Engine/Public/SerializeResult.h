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
	return MagicEnumToStringView(error).data();
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
