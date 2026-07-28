#include "pch.h"
#include "CinematicSystem.h"

NS_USING(Engine)

CCinematicSystem::CCinematicSystem(CCameraManager& CameraManager)
	: m_CameraManager{CameraManager}
{
}

CCinematicSystem::~CCinematicSystem()
{

}

HRESULT CCinematicSystem::Initialize(const StringID& CinematicCameraID)
{
	m_CinematicCameraID = CinematicCameraID;

	return S_OK;
}

void CCinematicSystem::Update(_float fTimeDelta)
{

}


UPtr<CCinematicSystem> CCinematicSystem::Create(CCameraManager& CameraManager, const StringID& CinematicCameraID)
{
	auto pInstance = ToUPtr(new CCinematicSystem(CameraManager));
	if (FAILED(pInstance->Initialize(CinematicCameraID)))
	{
		return nullptr;
	}
	return pInstance;
}
