#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Engine
{

class CHandle {
public:
	static constexpr uint32_t INVALID_INDEX =
		std::numeric_limits<uint32_t>::max();
	static constexpr uint32_t INVALID_GENERATION =
		std::numeric_limits<uint32_t>::max();

	constexpr CHandle() = default;
	explicit constexpr CHandle(uint32_t iIndex, uint32_t iGeneration)
		: m_iIndex{ iIndex }, m_iGeneration{ iGeneration }
	{
	}

	constexpr bool operator==(const CHandle& other) const
	{
		return m_iIndex == other.m_iIndex && m_iGeneration == other.m_iGeneration;
	}

	constexpr bool IsValid() const { return m_iIndex != INVALID_INDEX; }
	constexpr uint32_t GetIndex() const { return m_iIndex; }
	constexpr uint32_t GetGeneration() const { return m_iGeneration; }
	constexpr uint64_t GetPackedValue() const
	{
		return (static_cast<uint64_t>(m_iGeneration) << 32u) |
			static_cast<uint64_t>(m_iIndex);
	}

private:
	uint32_t m_iIndex{ INVALID_INDEX };
	uint32_t m_iGeneration{ INVALID_GENERATION };
};

static_assert(sizeof(CHandle) == sizeof(uint64_t));

}
