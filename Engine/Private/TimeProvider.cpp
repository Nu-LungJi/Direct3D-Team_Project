#include "pch.h"
#include "TimeProvider.h"

NS_USING(Engine)

CTimeProvider::CTimeProvider()
{

}

CTimeProvider::~CTimeProvider()
{
}

HRESULT CTimeProvider::Initialize()
{
	QueryPerformanceCounter(&m_FrameTime);
	QueryPerformanceCounter(&m_LastTime);
	QueryPerformanceCounter(&m_FixTime);

	QueryPerformanceFrequency(&m_CpuTick);
	return S_OK;
}

UPtr<CTimeProvider> CTimeProvider::Create()
{
	auto pInstance = UPtr<CTimeProvider>(new CTimeProvider{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("CTimeProvider Create Failed");
		return nullptr;
	}
	return pInstance;
}
