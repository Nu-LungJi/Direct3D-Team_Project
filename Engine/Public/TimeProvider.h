#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CTimeProvider final: public CEngineBase
{
private:
	CTimeProvider();
	~CTimeProvider();

public:
	_float GetTimeDelta() const
	{
		return m_fTimeDelta;
	}

public:
	_float UpdateTimeProvider()
	{
		QueryPerformanceCounter(&m_FrameTime);

		if (m_FrameTime.QuadPart - m_FixTime.QuadPart >= m_CpuTick.QuadPart)
		{
			QueryPerformanceFrequency(&m_CpuTick);
			m_FixTime = m_FrameTime;
		}

		m_fTimeDelta = (m_FrameTime.QuadPart - m_LastTime.QuadPart) / static_cast<_float>(m_CpuTick.QuadPart);

		m_LastTime = m_FrameTime;

		return m_fTimeDelta;
	}

private:
	HRESULT Initialize();

public:
	static UPtr<CTimeProvider> Create();

private:
	LARGE_INTEGER m_FrameTime{};
	LARGE_INTEGER m_FixTime{};
	LARGE_INTEGER m_LastTime{};
	LARGE_INTEGER m_CpuTick{};

	_float m_fTimeDelta{};
};

NS_END