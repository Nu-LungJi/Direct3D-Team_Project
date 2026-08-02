#include "pch.h"
#include "ResPathPlayback.h"

#include "GameInstance.h"

NS_USING(Engine)

static constexpr _float RES_PATH_PLAYBACK_EPSILON = 0.0001f;
static constexpr _float RES_PATH_PLAYBACK_QUATERNION_EPSILON = 1.e-8f;
static constexpr const char* RES_PATH_PLAYBACK_ROOT = "PathPlayback";

static void ResPathPlaybackAddValidationError(
	std::vector<std::string>* pOutErrors,
	std::string Error)
{
	if (pOutErrors)
		pOutErrors->push_back(std::move(Error));
}

template <typename TEnum>
static _bool ResPathPlaybackIsValidEnum(TEnum Value)
{
	return !magic_enum::enum_name(Value).empty();
}

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
	if (!ValidateAndNormalizeData(Data) || !BuildLookup(Data))
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

_bool CResPathPlayback::ValidateAndNormalizeData(
	PATH_PLAYBACK_DATA& Data,
	std::vector<std::string>* pOutErrors)
{
	if (pOutErrors)
		pOutErrors->clear();

	Data.SortKeyframes();
	_bool bValid = true;

	if (Data.iVersion != PATH_PLAYBACK_DATA_VERSION)
	{
		ResPathPlaybackAddValidationError(
			pOutErrors,
			"Unsupported PathPlayback version. Expected " +
			std::to_string(PATH_PLAYBACK_DATA_VERSION) +
			", received " + std::to_string(Data.iVersion) + ".");
		bValid = false;
	}

	if (Data.Clips.empty())
	{
		ResPathPlaybackAddValidationError(
			pOutErrors,
			"PathPlayback data must contain at least one clip.");
		return false;
	}

	std::unordered_set<StringID> ClipIDs{};
	ClipIDs.reserve(Data.Clips.size());
	for (size_t iClip = 0; iClip < Data.Clips.size(); ++iClip)
	{
		auto& Clip = Data.Clips[iClip];
		const std::string ClipLabel =
			"Clip[" + std::to_string(iClip) + "]";

		if (Clip.sClipID.hash == 0)
		{
			ResPathPlaybackAddValidationError(
				pOutErrors, ClipLabel + " has an empty ClipID.");
			bValid = false;
		}
		else if (!ClipIDs.emplace(Clip.sClipID).second)
		{
			ResPathPlaybackAddValidationError(
				pOutErrors,
				ClipLabel + " has a duplicated ClipID: " +
				Clip.sClipID.GetDbgStr() + ".");
			bValid = false;
		}

		if (!ResPathPlaybackIsValidEnum(Clip.eCoordinateSpace) ||
			!ResPathPlaybackIsValidEnum(Clip.eRotationMode) ||
			!ResPathPlaybackIsValidEnum(Clip.ePlayMode) ||
			!ResPathPlaybackIsValidEnum(Clip.eFinishBehavior))
		{
			ResPathPlaybackAddValidationError(
				pOutErrors, ClipLabel + " contains an invalid enum value.");
			bValid = false;
		}

		if (Clip.Keyframes.size() < 2)
		{
			ResPathPlaybackAddValidationError(
				pOutErrors,
				ClipLabel + " must contain at least two keyframes.");
			bValid = false;
		}

		for (size_t iKeyframe = 0;
			iKeyframe < Clip.Keyframes.size(); ++iKeyframe)
		{
			auto& Keyframe = Clip.Keyframes[iKeyframe];
			const std::string KeyframeLabel =
				ClipLabel + ".Keyframe[" +
				std::to_string(iKeyframe) + "]";
			if (!std::isfinite(Keyframe.fTime) ||
				!ResPathPlaybackIsFinite(Keyframe.vPosition) ||
				!ResPathPlaybackIsFinite(Keyframe.vRotation))
			{
				ResPathPlaybackAddValidationError(
					pOutErrors,
					KeyframeLabel + " contains a non-finite value.");
				bValid = false;
				continue;
			}

			if (Keyframe.fTime < 0.f)
			{
				ResPathPlaybackAddValidationError(
					pOutErrors,
					KeyframeLabel + " has a negative time.");
				bValid = false;
			}

			if (!ResPathPlaybackIsValidEnum(
					Keyframe.ePositionInterpolation) ||
				!ResPathPlaybackIsValidEnum(Keyframe.eEasing))
			{
				ResPathPlaybackAddValidationError(
					pOutErrors,
					KeyframeLabel + " contains an invalid enum value.");
				bValid = false;
			}

			const _vector Rotation =
				XMLoadFloat4(&Keyframe.vRotation);
			const _float fRotationLengthSq =
				XMVectorGetX(XMQuaternionLengthSq(Rotation));
			if (fRotationLengthSq <=
				RES_PATH_PLAYBACK_QUATERNION_EPSILON)
			{
				ResPathPlaybackAddValidationError(
					pOutErrors,
					KeyframeLabel + " has a zero-length rotation.");
				bValid = false;
			}
			else
			{
				XMStoreFloat4(
					&Keyframe.vRotation,
					XMQuaternionNormalize(Rotation));
			}

			if (iKeyframe > 0 &&
				Keyframe.fTime -
					Clip.Keyframes[iKeyframe - 1].fTime <=
				RES_PATH_PLAYBACK_EPSILON)
			{
				ResPathPlaybackAddValidationError(
					pOutErrors,
					KeyframeLabel +
					" overlaps the previous keyframe time.");
				bValid = false;
			}
		}
	}

	return bValid;
}

_bool CResPathPlayback::BuildLookup(const PATH_PLAYBACK_DATA& Data)
{
	std::unordered_map<StringID, size_t> NewLookup{};
	NewLookup.reserve(Data.Clips.size());
	for (size_t iClip = 0; iClip < Data.Clips.size(); ++iClip)
	{
		if (!NewLookup.emplace(Data.Clips[iClip].sClipID, iClip).second)
			return false;
	}

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
