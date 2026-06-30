#pragma once
#include "Engine_Defines.h"
#include "Handle.h"
#include "CameraObject.h"
NS_BEGIN(Engine)

class CCameraManager final : public CEngineBase
{
private:
	CCameraManager();
	~CCameraManager() override;

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

//public:
//	CCameraObject* GetActiveGameCamera() const;
//	HRESULT SetActiveGameCamera(const StringID& CameraID);
//	CCameraObject* GetActiveUICamera() const;
//	HRESULT SetActiveUICamera(const StringID& CameraID);
//
//	CCameraObject* GetActiveGameCamera(const StringID& CameraID) const;
//	CCameraObject* GetActiveUICamera(const StringID& CameraID) const;
//
//
//
//	CCameraObject* GetGameCamera(const StringID& CameraID) const;
//	CCameraObject* GetUICamera(const StringID& CameraID) const;
//	HRESULT RegistGameCamera(const StringID& CameraID, const CHandle& handle);
//	HRESULT RegistUICamera(const StringID& CameraID, const CHandle& handle);
//
//private:
//	std::optional<std::pair<StringID, CHandle>> m_ActiveGameCamera{};
//	std::optional<std::pair<StringID, CHandle>> m_ActiveUICamera{};
//
//	std::unordered_map<StringID, CHandle> m_GameCameras{};
//	std::unordered_map<StringID, CHandle> m_UICameras{};

public:
	static UPtr<CCameraManager> Create();
};

NS_END