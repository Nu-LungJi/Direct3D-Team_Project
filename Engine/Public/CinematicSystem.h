#pragma once
#include "Engine_Base.h"

NS_BEGIN(Engine)

class CCameraManager;
class CCinematicAsset;

class ENGINE_DLL CCinematicSystem final : public CEngineBase
{
private:
	CCinematicSystem(CCameraManager& CameraManager);
	~CCinematicSystem() override;
private:
	HRESULT Initialize(const StringID& CinematicCameraID);

public:
	void Update(_float fTimeDelta);

public:
	HRESULT Load(const StringID& CinematicID, const std::string& filepath);
	HRESULT Play(const StringID& CinematicID);

	void Pause();
	void Resume();
	void Stop();
	void Seek(_float fTime);

	_bool IsPlaying() const;

private:
	// CGameInstance 반복호출보단 그냥 가져와 씀
	CCameraManager& m_CameraManager;


	// 시네마틱 재생해서 보여주는 카메라
	StringID m_CinematicCameraID { "CinematicCamera" };
	// 시네마틱 재생 전 카메라
	StringID m_PreviousCameraID {};


	
	std::unordered_map<StringID, SPtr<CCinematicAsset>> m_Assets{};
public:
	static UPtr<CCinematicSystem> Create(CCameraManager& CameraManager, const StringID& CinematicCameraID);
};

NS_END
