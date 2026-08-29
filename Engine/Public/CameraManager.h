#pragma once
#include "Engine_Defines.h"
#include "Handle.h"
#include "CameraObject.h"
#include "CinematicTypes.h"
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

	void SetCinematicCollisionQueryMask(uint32_t iQueryMask);
private:
	std::optional<std::pair<StringID, CHandle>> m_ActiveCamera{};
	std::unordered_map<StringID, CHandle> m_Cameras{};

#pragma region CINEMATIC
public:
	HRESULT BeginCinematicCamera();
	HRESULT EndCinematicCamera();
	_bool IsCinematicCameraActive() const;

	HRESULT RegistCinematicAsset(const SPtr<CCinematicAsset>& pAsset);
	HRESULT LoadCinematic(const std::string& CinematicName);
	HRESULT PlayCinematic(const StringID& CinematicID, const FCinematicPlayOptions& Options = {});
	HRESULT PlayCinematic(const StringID& CinematicID, const CHandle& TargetHandle, const FCinematicPlayOptions& Options = {});
	void StopCinematic(_float fReturnBlendDuration = 0.f);
	_bool IsCinematicPlaying() const;
	_float GetCinematicPlayTime() const;

#pragma endregion

private:
	UPtr<CCinematicSystem> m_pCinematicSystem;

public:
	static UPtr<CCameraManager> Create();
};

NS_END
