#pragma once

#include <cstddef>
#include <cstdint>

namespace Engine::BinSerializeFormat
{
	inline constexpr uint32_t MAGIC = 0x4E42534A; // "JSBN" in little endian
	inline constexpr uint16_t VERSION = 1;
	inline constexpr uint16_t FLAGS = 0;

	inline constexpr uint64_t MAX_PAYLOAD_BYTES = 512ull * 1024ull * 1024ull;
	inline constexpr uint64_t MAX_STRING_BYTES = 16ull * 1024ull * 1024ull;
	inline constexpr uint64_t MAX_CONTAINER_ELEMENTS = 1'000'000ull;

	struct HEADER
	{
		uint32_t iMagic{ MAGIC };
		uint16_t iVersion{ VERSION };
		uint16_t iFlags{ FLAGS };
		uint64_t iPayloadSize{};
	};

	static_assert(sizeof(HEADER) == 16);
}
