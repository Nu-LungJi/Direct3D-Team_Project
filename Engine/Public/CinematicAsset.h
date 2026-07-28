#pragma once
#include "Engine_Base.h"
#include "CinematicTypes.h"
#include "ISerializable.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCinematicAsset final : public CEngineBase
{
private:
	explicit CCinematicAsset(const StringID& CinematicID);
	~CCinematicAsset() override;

public:
	const StringID& GetCinematicID() const { return m_CinematicID; }
	_float GetDuration() const { return m_fDuration; }

	const FCinematicCameraTrack& GetCameraTrack() const
	{
		return m_CameraTrack;
	}

	FCinematicCameraTrack& GetMutableCameraTrack()
	{
		return m_CameraTrack;
	}

	FCinematicAssetData ExportData() const;
	void RecalculateDuration();

private:
	StringID m_CinematicID{};
	_float m_fDuration{};
	FCinematicCameraTrack m_CameraTrack{};

public:
	static SPtr<CCinematicAsset> Create(const StringID& CinematicID);
	static SPtr<CCinematicAsset> Create(const FCinematicAssetData& Data);
};

NS_END
