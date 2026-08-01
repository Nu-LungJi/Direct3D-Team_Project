#include "pch.h"
#include "ResPathPlayback.h"

#include "GameInstance.h"

NS_USING(Engine)

static constexpr _float RES_PATH_PLAYBACK_EPSILON = 0.0001f;
static constexpr const char* RES_PATH_PLAYBACK_ROOT = "PathPlayback";

static _bool ResPathPlaybackIsFinite(const _float3& Value)
{
	return std::isfinite(Value.x) &&
		std::isfinite(Value.y) &&
		std::isfinite(Value.z);
}

static _bool ResPathPlaybackIsFinite(const _float4& Value)
{
	return std::isfinite(Value.x) &&
		std::isfinite(Value.y) &&
		std::isfinite(Value.z) &&
		std::isfinite(Value.w);
}

CResPathPlayback::CResPathPlayback(const _string& sPath)
	: CResource{ sPath }
{
}

CResPathPlayback::~CResPathPlayback() = default;

HRESULT CResPathPlayback::Load(const std::any& arg)
{
	if (m_eState.load(std::memory_order_acquire) == STATE::LOADED)
		return S_OK;
	if (m_sPath.empty())
		return E_INVALIDARG;

	m_eState.store(STATE::LOADING, std::memory_order_release);

	PATH_PLAYBACK_DATA LoadedData{};
	_string Extension = std::filesystem::path{ m_sPath }.extension().string();
	std::transform(
		Extension.begin(), Extension.end(), Extension.begin(),
		[](unsigned char Character)
		{
			return static_cast<char>(std::tolower(Character));
		});

	HRESULT hResult = E_INVALIDARG;
	if (Extension == ".json")
	{
		hResult = CGameInstance::Get().JsonDeSerialize(
			m_sPath, LoadedData, RES_PATH_PLAYBACK_ROOT, false);
	}
	else if (Extension == ".bin")
	{
		hResult = CGameInstance::Get().BinDeSerialize(
			m_sPath, LoadedData, RES_PATH_PLAYBACK_ROOT, false);
	}

	if (FAILED(hResult) || FAILED(SetData(std::move(LoadedData))))
	{
		m_Data = {};
		m_ClipLookup.clear();
		m_eState.store(STATE::LOADFAIL, std::memory_order_release);
		DEBUG_LOG("[PathPlayback] Resource load failed.\n");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CResPathPlayback::Unload(const std::any& arg)
{
	m_Data = {};
	m_ClipLookup.clear();
	m_eState.store(STATE::UNLOAD, std::memory_order_release);
	return S_OK;
}

HRESULT CResPathPlayback::SetData(PATH_PLAYBACK_DATA Data)
{
	if (!ValidateAndBuildLookup(Data))
		return E_INVALIDARG;

	m_Data = std::move(Data);
	m_eState.store(STATE::LOADED, std::memory_order_release);
	return S_OK;
}

const PATH_PLAYBACK_CLIP* CResPathPlayback::FindClip(
	const StringID& sClipID) const
{
	const auto Index = FindClipIndex(sClipID);
	return Index ? &m_Data.Clips[*Index] : nullptr;
}

std::optional<size_t> CResPathPlayback::FindClipIndex(
	const StringID& sClipID) const
{
	const auto Iter = m_ClipLookup.find(sClipID);
	if (Iter == m_ClipLookup.end())
		return std::nullopt;
	return Iter->second;
}

_float CResPathPlayback::GetClipDuration(const StringID& sClipID) const
{
	const auto* pClip = FindClip(sClipID);
	if (!pClip || pClip->Keyframes.size() < 2)
		return 0.f;
	return pClip->Keyframes.back().fTime -
		pClip->Keyframes.front().fTime;
}

_bool CResPathPlayback::ValidateAndBuildLookup(PATH_PLAYBACK_DATA& Data)
{
	Data.SortKeyframes();

	std::unordered_map<StringID, size_t> NewLookup{};
	NewLookup.reserve(Data.Clips.size());
	for (size_t iClip = 0; iClip < Data.Clips.size(); ++iClip)
	{
		const auto& Clip = Data.Clips[iClip];
		if (Clip.sClipID.hash == 0 || Clip.Keyframes.size() < 2 ||
			!NewLookup.emplace(Clip.sClipID, iClip).second)
		{
			return false;
		}

		for (size_t iKeyframe = 0;
			iKeyframe < Clip.Keyframes.size(); ++iKeyframe)
		{
			const auto& Keyframe = Clip.Keyframes[iKeyframe];
			if (!std::isfinite(Keyframe.fTime) ||
				!ResPathPlaybackIsFinite(Keyframe.vPosition) ||
				!ResPathPlaybackIsFinite(Keyframe.vRotation))
			{
				return false;
			}

			if (iKeyframe > 0 &&
				Keyframe.fTime -
					Clip.Keyframes[iKeyframe - 1].fTime <=
				RES_PATH_PLAYBACK_EPSILON)
			{
				return false;
			}
		}
	}

	if (Data.Clips.empty())
		return false;

	m_ClipLookup = std::move(NewLookup);
	return true;
}

SPtr<CResPathPlayback> CResPathPlayback::Create(const _string& sPath)
{
	return ToSPtr(new CResPathPlayback{ sPath });
}

SPtr<CResPathPlayback> CResPathPlayback::CreateAndLoad(
	const _string& sPath)
{
	auto pInstance = Create(sPath);
	if (!pInstance || FAILED(pInstance->Load()))
		return nullptr;
	return pInstance;
}

SPtr<CResPathPlayback> CResPathPlayback::CreateFromData(
	PATH_PLAYBACK_DATA Data)
{
	auto pInstance = Create({});
	if (!pInstance || FAILED(pInstance->SetData(std::move(Data))))
		return nullptr;
	return pInstance;
}
