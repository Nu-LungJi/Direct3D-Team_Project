#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace Engine
{

class CHandle {
public:
	CHandle() {}
	explicit CHandle(size_t iIndex, uint32_t iGeneration) : m_iIndex{ iIndex }, m_iGeneration{ iGeneration } {}

	bool operator==(const CHandle& other) const
	{
		return m_iIndex == other.m_iIndex && m_iGeneration == other.m_iGeneration;
	}

	size_t GetIndex() const { return m_iIndex; }
	uint32_t GetGeneration() const { return m_iGeneration; }

private:
	size_t m_iIndex{ std::numeric_limits<size_t>::max() };
	uint32_t m_iGeneration{ std::numeric_limits<uint32_t>::max() };
};

}
