#pragma once
#include "Engine_Defines.h"
#include "Handle.h"
#include "CameraObject.h"
NS_BEGIN(Engine)

class CCinematicSystem;
class CCinematicAsset;

class CCameraManager final : public CEngineBase
{
private:
	CCameraManager();
	~CCameraManager() override;
private:
	HRESULT Initialize();

public:
	void Update(_float fTimeDelta);
	void UpdateGUI();

public:
	CCameraObject* GetActiveCamera() const;
	CCameraObject* GetActiveCamera(const StringID& CameraID) const;
	std::optional<StringID> GetActiveCameraID() const;
	HRESULT SetActiveCamera(const StringID& CameraID);

	CCameraObject* GetCamera(const StringID& CameraID) const;
	HRESULT RegistCamera(const StringID& CameraID, const CHandle& handle);

private:
	std::optional<std::pair<StringID, CHandle>> m_ActiveCamera{};
	std::unordered_map<StringID, CHandle> m_Cameras{};

#pragma region CINEMATIC
public:
	HRESULT BeginCinematicCamera();
	HRESULT EndCinematicCamera();
	_bool IsCinematicCameraActive() const;

	HRESULT RegistCinematicAsset(const SPtr<CCinematicAsset>& pAsset);
	HRESULT LoadCinematic(const StringID& CinematicID, const std::string& filepath);
	HRESULT PlayCinematic(const StringID& CinematicID);
	void StopCinematic();
	_bool IsCinematicPlaying() const;
	_float GetCinematicPlayTime() const;

#pragma endregion

private:
	UPtr<CCinematicSystem> m_pCinematicSystem;

public:
	static UPtr<CCameraManager> Create();
};

NS_END
