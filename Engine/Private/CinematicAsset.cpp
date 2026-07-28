#include "pch.h"
#include "CinematicAsset.h"

NS_USING(Engine)

CCinematicAsset::CCinematicAsset(const StringID& CinematicID)
	: m_CinematicID{ CinematicID }
{
}

CCinematicAsset::~CCinematicAsset()
{
}

FCinematicAssetData CCinematicAsset::ExportData() const
{
	FCinematicAssetData Data{};
	Data.CinematicID = m_CinematicID;
	Data.CameraTrack = m_CameraTrack;
	return Data;
}

void CCinematicAsset::RecalculateDuration()
{
	m_fDuration = 0.f;

	for (const auto& Shot : m_CameraTrack.Shots)
	{
		_float fShotDuration{};
		for (const auto& Keyframe : Shot.Keyframes)
		{
			fShotDuration = std::max(fShotDuration, Keyframe.fTime);
		}

		m_fDuration = std::max(m_fDuration, Shot.fStartTime + fShotDuration);
	}
}

SPtr<CCinematicAsset> CCinematicAsset::Create(const StringID& CinematicID)
{
	return ToSPtr<CCinematicAsset>(new CCinematicAsset{ CinematicID });
}

SPtr<CCinematicAsset> CCinematicAsset::Create(const FCinematicAssetData& Data)
{
	if (Data.CinematicID.GetDbgStr()[0] == '\0')
	{
		return nullptr;
	}

	auto pInstance = ToSPtr<CCinematicAsset>(new CCinematicAsset{ Data.CinematicID });
	pInstance->m_CameraTrack = Data.CameraTrack;
	pInstance->RecalculateDuration();
	return pInstance;
}
