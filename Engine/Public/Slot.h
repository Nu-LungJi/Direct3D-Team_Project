#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

template<typename T>
class CSlot {
public:
	CSlot() = default;
	explicit CSlot(UPtr<T> p) : m_p{ std::move(p) } {}
public:
	uint32_t GetGeneration() const { return m_iGeneration; }
	const T* Get() const { return m_p.get(); }
	T* Get() { return m_p.get(); }
	bool IsOccupied() const { return m_p != nullptr; }
	void Reset()
	{
		m_p.reset();
		GenerationIncrease();
	}
	void Set(UPtr<T> p) { m_p = std::move(p); }
private:
	void GenerationIncrease() { ++m_iGeneration; };
private:
	UPtr<T> m_p{};
	uint32_t m_iGeneration{};
};

NS_END