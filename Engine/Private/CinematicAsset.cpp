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
