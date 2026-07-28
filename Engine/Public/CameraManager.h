#pragma once
#include "Engine_Defines.h"
#include "Handle.h"
#include "CameraObject.h"
NS_BEGIN(Engine)

class CCinematicSystem;

class CCameraManager final : public CEngineBase
{
private:
	CCameraManager();
	~CCameraManager() override;
private:
	HRESULT Initialize();

public:
	void UpdateGUI();

public:
	CCameraObject* GetActiveCamera() const;
	CCameraObject* GetActiveCamera(const StringID& CameraID) const;
	HRESULT SetActiveCamera(const StringID& CameraID);

	CCameraObject* GetCamera(const StringID& CameraID) const;
	HRESULT RegistCamera(const StringID& CameraID, const CHandle& handle);

private:
	std::optional<std::pair<StringID, CHandle>> m_ActiveCamera{};
	std::unordered_map<StringID, CHandle> m_Cameras{};
#pragma region CINEMATIC
public:


#pragma endregion

private:
	UPtr<CCinematicSystem> m_pCinematicSystem;

public:
	static UPtr<CCameraManager> Create();
};

NS_END
