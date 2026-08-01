#include "pch.h"
#include "ComPathPlayback.h"

#include "GameObject.h"
#include "ResPathPlayback.h"

NS_USING(Engine)

static constexpr _float COM_PATH_PLAYBACK_EPSILON = 0.0001f;
static constexpr uint32_t COM_PATH_PLAYBACK_MAX_BOUNDARY_CROSSINGS = 1024u;

static _float ComPathPlaybackApplyEasing(
	_float fRatio,
	PATH_PLAYBACK_EASING eEasing)
{
	fRatio = std::clamp(fRatio, 0.f, 1.f);

	switch (eEasing)
	{
	case PATH_PLAYBACK_EASING::EASE_IN:
		return fRatio * fRatio;

	case PATH_PLAYBACK_EASING::EASE_OUT:
	{
		const _float fInverse = 1.f - fRatio;
		return 1.f - fInverse * fInverse;
	}

	case PATH_PLAYBACK_EASING::EASE_IN_OUT:
		return fRatio * fRatio * (3.f - 2.f * fRatio);

	case PATH_PLAYBACK_EASING::LINEAR:
	default:
		return fRatio;
	}
}

static _vector ComPathPlaybackNormalizeQuaternionOrIdentity(
	const _float4& Rotation)
{
	const _vector Quaternion = XMLoadFloat4(&Rotation);
	const _float fLengthSq = XMVectorGetX(XMQuaternionLengthSq(Quaternion));
	if (!std::isfinite(fLengthSq) ||
		fLengthSq <= COM_PATH_PLAYBACK_EPSILON)
	{
		return XMQuaternionIdentity();
	}
	return XMQuaternionNormalize(Quaternion);
}

static _float4 ComPathPlaybackMakeFacingQuaternion(
	const _float3& Direction,
	const _float4& Fallback)
{
	_vector Look = XMLoadFloat3(&Direction);
	if (XMVectorGetX(XMVector3LengthSq(Look)) <=
		COM_PATH_PLAYBACK_EPSILON)
	{
		return Fallback;
	}

	Look = XMVector3Normalize(Look);
	_vector Up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	if (std::abs(XMVectorGetX(XMVector3Dot(Look, Up))) >= 0.999f)
		Up = XMVectorSet(0.f, 0.f, 1.f, 0.f);

	const _vector Right = XMVector3Normalize(XMVector3Cross(Up, Look));
	Up = XMVector3Cross(Look, Right);

	_matrix RotationMatrix = XMMatrixIdentity();
	RotationMatrix.r[0] = XMVectorSetW(Right, 0.f);
	RotationMatrix.r[1] = XMVectorSetW(Up, 0.f);
	RotationMatrix.r[2] = XMVectorSetW(Look, 0.f);

	_float4 Result{};
	XMStoreFloat4(
		&Result,
		XMQuaternionNormalize(
			XMQuaternionRotationMatrix(RotationMatrix)));
	return Result;
}

CComPathPlayback::CComPathPlayback()
{
}

CComPathPlayback::CComPathPlayback(const CComPathPlayback& Prototype)
	: CComponent{ Prototype }
{
}

CComPathPlayback::~CComPathPlayback() = default;

HRESULT CComPathPlayback::Initialize(void* pArg)
{
	if (!pArg || FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	const auto* pDesc = static_cast<const DESC*>(pArg);
	SetPlaybackRate(pDesc->fPlaybackRate);
	if (pDesc->pPathResource &&
		!SetPathResource(pDesc->pPathResource))
	{
		return E_FAIL;
	}

	return S_OK;
}

_bool CComPathPlayback::SetPathResource(
	SPtr<CResPathPlayback> pPathResource)
{
	if (!pPathResource ||
		pPathResource->GetState() != CResource::STATE::LOADED)
	{
		return false;
	}

	m_pPathResource = std::move(pPathResource);
	ResetRuntimeState();
	return true;
}

_bool CComPathPlayback::Play(
	const StringID& sClipID,
	PATH_PLAYBACK_DIRECTION eDirection,
	_bool bRestart)
{
	if (!m_pPathResource || !m_pGameObject)
		return false;

	const auto* pClip = m_pPathResource->FindClip(sClipID);
	if (!pClip)
		return false;

	if (!bRestart && m_sCurrentClipID == sClipID)
	{
		if (m_eState == PATH_PLAYBACK_STATE::PAUSED)
			Resume();
		return m_eState == PATH_PLAYBACK_STATE::PLAYING;
	}

	m_sCurrentClipID = sClipID;
	m_eDirection = eDirection;
	m_eInitialDirection = eDirection;
	m_eState = PATH_PLAYBACK_STATE::PLAYING;
	m_fElapsedTime = eDirection == PATH_PLAYBACK_DIRECTION::FORWARD
		? 0.f
		: GetDuration();

	m_tStartAnchorPose.vPosition =
		m_pGameObject->GetTransform().GetPosition();
	m_tStartAnchorPose.vRotation =
		m_pGameObject->GetTransform().GetQuaternion();

	m_ReachedKeyframeIndicesThisCommit.clear();
	const size_t iStartKeyframe =
		eDirection == PATH_PLAYBACK_DIRECTION::FORWARD
		? 0
		: pClip->Keyframes.size() - 1;
	m_ReachedKeyframeIndicesThisCommit.push_back(iStartKeyframe);

	DiscardEvaluatedStep();
	return EvaluatePose(*pClip, m_fElapsedTime, m_tCurrentPose);
}

void CComPathPlayback::Pause()
{
	if (m_eState == PATH_PLAYBACK_STATE::PLAYING)
	{
		m_eState = PATH_PLAYBACK_STATE::PAUSED;
		DiscardEvaluatedStep();
	}
}

void CComPathPlayback::Resume()
{
	if (m_eState == PATH_PLAYBACK_STATE::PAUSED)
		m_eState = PATH_PLAYBACK_STATE::PLAYING;
}

void CComPathPlayback::Stop()
{
	ResetRuntimeState();
}

void CComPathPlayback::Interrupt()
{
	if (m_eState == PATH_PLAYBACK_STATE::PLAYING ||
		m_eState == PATH_PLAYBACK_STATE::PAUSED)
	{
		m_eState = PATH_PLAYBACK_STATE::INTERRUPTED;
	}
	DiscardEvaluatedStep();
}

PATH_PLAYBACK_STEP_RESULT CComPathPlayback::EvaluateNext(
	_float fFixedTimeDelta)
{
	DiscardEvaluatedStep();
	if (m_eState != PATH_PLAYBACK_STATE::PLAYING ||
		fFixedTimeDelta <= 0.f || m_fPlaybackRate <= 0.f)
	{
		return {};
	}

	const auto* pClip = GetCurrentClip();
	if (!pClip)
		return {};

	m_fPendingAdvanceTime = fFixedTimeDelta * m_fPlaybackRate;
	const CURSOR_RESULT Cursor =
		AdvanceCursor(*pClip, m_fPendingAdvanceTime, false);

	PATH_PLAYBACK_STEP_RESULT Step{};
	Step.bValid = EvaluatePose(
		*pClip,
		Cursor.fElapsedTime,
		Step.tTargetPose,
		&Step.iTargetSegmentIndex);
	Step.fStartElapsedTime = m_fElapsedTime;
	Step.fTargetElapsedTime = Cursor.fElapsedTime;
	Step.eTargetDirection = Cursor.eDirection;
	Step.bWouldComplete =
		Cursor.eState == PATH_PLAYBACK_STATE::COMPLETED;
	Step.bWouldWrap = Cursor.bWrapped;

	if (!Step.bValid)
	{
		m_fPendingAdvanceTime = 0.f;
		return {};
	}

	m_tPendingStep = Step;
	m_bHasPendingEvaluation = true;
	return Step;
}

_bool CComPathPlayback::CommitEvaluatedStep(_float fAcceptedRatio)
{
	if (!m_bHasPendingEvaluation)
		return false;

	const auto* pClip = GetCurrentClip();
	if (!pClip)
	{
		DiscardEvaluatedStep();
		return false;
	}

	fAcceptedRatio = std::clamp(fAcceptedRatio, 0.f, 1.f);
	m_ReachedKeyframeIndicesThisCommit.clear();
	const CURSOR_RESULT Cursor = AdvanceCursor(
		*pClip,
		m_fPendingAdvanceTime * fAcceptedRatio,
		true);

	m_fElapsedTime = Cursor.fElapsedTime;
	m_eDirection = Cursor.eDirection;
	m_eState = Cursor.eState;
	const _bool bEvaluated = EvaluatePose(
		*pClip, m_fElapsedTime, m_tCurrentPose);

	DiscardEvaluatedStep();
	return bEvaluated;
}

void CComPathPlayback::DiscardEvaluatedStep()
{
	m_tPendingStep = {};
	m_fPendingAdvanceTime = 0.f;
	m_bHasPendingEvaluation = false;
}

const PATH_PLAYBACK_CLIP* CComPathPlayback::GetCurrentClip() const
{
	return m_pPathResource
		? m_pPathResource->FindClip(m_sCurrentClipID)
		: nullptr;
}

_float CComPathPlayback::GetDuration() const
{
	return m_pPathResource
		? m_pPathResource->GetClipDuration(m_sCurrentClipID)
		: 0.f;
}

_float CComPathPlayback::GetNormalizedTime() const
{
	const _float fDuration = GetDuration();
	return fDuration > COM_PATH_PLAYBACK_EPSILON
		? std::clamp(m_fElapsedTime / fDuration, 0.f, 1.f)
		: 0.f;
}

void CComPathPlayback::SetPlaybackRate(_float fPlaybackRate)
{
	m_fPlaybackRate = std::max(0.f, fPlaybackRate);
}

const PATH_PLAYBACK_KEYFRAME* CComPathPlayback::GetKeyframe(
	size_t iKeyframeIndex) const
{
	const auto* pClip = GetCurrentClip();
	return pClip && iKeyframeIndex < pClip->Keyframes.size()
		? &pClip->Keyframes[iKeyframeIndex]
		: nullptr;
}

void CComPathPlayback::UpdateGUI()
{
	CComponent::UpdateGUI();
	ImGui::Text("State: %s", magic_enum::enum_name(m_eState).data());
	ImGui::Text("Clip: %s", m_sCurrentClipID.GetDbgStr());
	ImGui::Text("Time: %.3f / %.3f", m_fElapsedTime, GetDuration());
	ImGui::Text("Normalized: %.3f", GetNormalizedTime());
	ImGui::Text("Direction: %s", magic_enum::enum_name(m_eDirection).data());
	ImGui::Text("Pending Evaluation: %s",
		m_bHasPendingEvaluation ? "true" : "false");
	ImGui::Text("Reached This Commit: %zu",
		m_ReachedKeyframeIndicesThisCommit.size());
}

_bool CComPathPlayback::EvaluatePose(
	const PATH_PLAYBACK_CLIP& Clip,
	_float fElapsedTime,
	PATH_PLAYBACK_POSE& OutPose,
	size_t* pOutSegmentIndex) const
{
	if (Clip.Keyframes.size() < 2)
		return false;

	const auto& Keyframes = Clip.Keyframes;
	const _float fDuration =
		Keyframes.back().fTime - Keyframes.front().fTime;
	const _float fSampleTime = Keyframes.front().fTime +
		std::clamp(fElapsedTime, 0.f, fDuration);

	const auto Upper = std::upper_bound(
		Keyframes.begin(), Keyframes.end(), fSampleTime,
		[](_float fTime, const PATH_PLAYBACK_KEYFRAME& Keyframe)
		{
			return fTime < Keyframe.fTime;
		});
	const size_t iRight = Upper == Keyframes.end()
		? Keyframes.size() - 1
		: static_cast<size_t>(std::distance(Keyframes.begin(), Upper));
	const size_t iLeft = iRight > 0 ? iRight - 1 : 0;

	const auto& Left = Keyframes[iLeft];
	const auto& Right = Keyframes[iRight];
	const _float fSegmentDuration = Right.fTime - Left.fTime;
	const _float fLinearRatio =
		fSegmentDuration > COM_PATH_PLAYBACK_EPSILON
		? std::clamp(
			(fSampleTime - Left.fTime) / fSegmentDuration,
			0.f, 1.f)
		: 0.f;
	const _float fRatio =
		ComPathPlaybackApplyEasing(fLinearRatio, Left.eEasing);

	_vector Position{};
	if (Left.ePositionInterpolation ==
		PATH_PLAYBACK_INTERPOLATION::CATMULL_ROM)
	{
		const size_t iPrevious = iLeft > 0 ? iLeft - 1 : iLeft;
		const size_t iNext = std::min(iRight + 1, Keyframes.size() - 1);
		Position = XMVectorCatmullRom(
			XMLoadFloat3(&Keyframes[iPrevious].vPosition),
			XMLoadFloat3(&Left.vPosition),
			XMLoadFloat3(&Right.vPosition),
			XMLoadFloat3(&Keyframes[iNext].vPosition),
			fRatio);
	}
	else
	{
		Position = XMVectorLerp(
			XMLoadFloat3(&Left.vPosition),
			XMLoadFloat3(&Right.vPosition),
			fRatio);
	}

	_vector Rotation = XMQuaternionSlerp(
		ComPathPlaybackNormalizeQuaternionOrIdentity(Left.vRotation),
		ComPathPlaybackNormalizeQuaternionOrIdentity(Right.vRotation),
		fRatio);

	if (Clip.eCoordinateSpace ==
		PATH_PLAYBACK_COORDINATE_SPACE::START_LOCAL)
	{
		const _vector AnchorRotation =
			ComPathPlaybackNormalizeQuaternionOrIdentity(
				m_tStartAnchorPose.vRotation);
		const _matrix AnchorWorld =
			XMMatrixRotationQuaternion(AnchorRotation) *
			XMMatrixTranslationFromVector(
				XMLoadFloat3(&m_tStartAnchorPose.vPosition));
		Position = XMVector3TransformCoord(Position, AnchorWorld);

		const _matrix WorldRotation =
			XMMatrixRotationQuaternion(Rotation) *
			XMMatrixRotationQuaternion(AnchorRotation);
		Rotation = XMQuaternionNormalize(
			XMQuaternionRotationMatrix(WorldRotation));
	}

	XMStoreFloat3(&OutPose.vPosition, Position);
	XMStoreFloat4(&OutPose.vRotation, Rotation);

	if (Clip.eRotationMode == PATH_PLAYBACK_ROTATION_MODE::KEEP &&
		m_pGameObject)
	{
		OutPose.vRotation =
			m_pGameObject->GetTransform().GetQuaternion();
	}
	else if (Clip.eRotationMode ==
		PATH_PLAYBACK_ROTATION_MODE::FACE_DIRECTION && m_pGameObject)
	{
		const _float3 vCurrentPosition =
			m_pGameObject->GetTransform().GetPosition();
		const _float3 vDirection{
			OutPose.vPosition.x - vCurrentPosition.x,
			OutPose.vPosition.y - vCurrentPosition.y,
			OutPose.vPosition.z - vCurrentPosition.z };
		OutPose.vRotation = ComPathPlaybackMakeFacingQuaternion(
			vDirection,
			m_pGameObject->GetTransform().GetQuaternion());
	}

	if (pOutSegmentIndex)
		*pOutSegmentIndex = iLeft;
	return true;
}

CComPathPlayback::CURSOR_RESULT CComPathPlayback::AdvanceCursor(
	const PATH_PLAYBACK_CLIP& Clip,
	_float fDeltaTime,
	_bool bCollectReachedKeyframes)
{
	CURSOR_RESULT Result{
		.fElapsedTime = m_fElapsedTime,
		.eDirection = m_eDirection,
		.eState = m_eState
	};
	const _float fDuration = GetDuration();
	if (Result.eState != PATH_PLAYBACK_STATE::PLAYING ||
		fDuration <= COM_PATH_PLAYBACK_EPSILON || fDeltaTime <= 0.f)
	{
		return Result;
	}

	_float fRemainingTime = fDeltaTime;
	uint32_t iBoundaryCrossings{};
	while (fRemainingTime > COM_PATH_PLAYBACK_EPSILON &&
		Result.eState == PATH_PLAYBACK_STATE::PLAYING &&
		iBoundaryCrossings++ < COM_PATH_PLAYBACK_MAX_BOUNDARY_CROSSINGS)
	{
		const _bool bForward =
			Result.eDirection == PATH_PLAYBACK_DIRECTION::FORWARD;
		const _float fBoundary = bForward ? fDuration : 0.f;
		const _float fTimeToBoundary = bForward
			? fDuration - Result.fElapsedTime
			: Result.fElapsedTime;
		const _float fTravelTime =
			std::min(fRemainingTime, std::max(0.f, fTimeToBoundary));
		const _float fPreviousElapsed = Result.fElapsedTime;
		Result.fElapsedTime += bForward ? fTravelTime : -fTravelTime;

		if (bCollectReachedKeyframes)
		{
			CollectReachedKeyframes(
				Clip,
				fPreviousElapsed,
				Result.fElapsedTime,
				Result.eDirection);
		}
		fRemainingTime -= fTravelTime;

		if (std::abs(Result.fElapsedTime - fBoundary) >
			COM_PATH_PLAYBACK_EPSILON)
		{
			break;
		}

		switch (Clip.ePlayMode)
		{
		case PATH_PLAYBACK_MODE::ONCE:
			Result.eState = PATH_PLAYBACK_STATE::COMPLETED;
			if (Clip.eFinishBehavior ==
				PATH_PLAYBACK_FINISH_BEHAVIOR::RESET_TO_START)
			{
				Result.fElapsedTime =
					m_eInitialDirection == PATH_PLAYBACK_DIRECTION::FORWARD
					? 0.f
					: fDuration;
			}
			break;

		case PATH_PLAYBACK_MODE::LOOP:
			Result.fElapsedTime = bForward ? 0.f : fDuration;
			Result.bWrapped = true;
			if (bCollectReachedKeyframes)
			{
				m_ReachedKeyframeIndicesThisCommit.push_back(
					bForward ? 0 : Clip.Keyframes.size() - 1);
			}
			break;

		case PATH_PLAYBACK_MODE::PING_PONG:
			Result.eDirection = bForward
				? PATH_PLAYBACK_DIRECTION::REVERSE
				: PATH_PLAYBACK_DIRECTION::FORWARD;
			Result.bWrapped = true;
			break;
		}
	}

	return Result;
}

void CComPathPlayback::CollectReachedKeyframes(
	const PATH_PLAYBACK_CLIP& Clip,
	_float fStartElapsed,
	_float fEndElapsed,
	PATH_PLAYBACK_DIRECTION eDirection)
{
	const _float fFirstTime = Clip.Keyframes.front().fTime;
	if (eDirection == PATH_PLAYBACK_DIRECTION::FORWARD)
	{
		for (size_t i = 0; i < Clip.Keyframes.size(); ++i)
		{
			const _float fKeyElapsed = Clip.Keyframes[i].fTime - fFirstTime;
			if (fKeyElapsed > fStartElapsed + COM_PATH_PLAYBACK_EPSILON &&
				fKeyElapsed <= fEndElapsed + COM_PATH_PLAYBACK_EPSILON)
			{
				m_ReachedKeyframeIndicesThisCommit.push_back(i);
			}
		}
	}
	else
	{
		for (size_t i = Clip.Keyframes.size(); i-- > 0;)
		{
			const _float fKeyElapsed = Clip.Keyframes[i].fTime - fFirstTime;
			if (fKeyElapsed < fStartElapsed - COM_PATH_PLAYBACK_EPSILON &&
				fKeyElapsed >= fEndElapsed - COM_PATH_PLAYBACK_EPSILON)
			{
				m_ReachedKeyframeIndicesThisCommit.push_back(i);
			}
		}
	}
}

void CComPathPlayback::ResetRuntimeState()
{
	m_sCurrentClipID = {};
	m_eState = PATH_PLAYBACK_STATE::IDLE;
	m_eDirection = PATH_PLAYBACK_DIRECTION::FORWARD;
	m_eInitialDirection = PATH_PLAYBACK_DIRECTION::FORWARD;
	m_fElapsedTime = 0.f;
	m_tStartAnchorPose = {};
	m_tStartAnchorPose.vRotation = { 0.f, 0.f, 0.f, 1.f };
	m_tCurrentPose = {};
	m_tCurrentPose.vRotation = { 0.f, 0.f, 0.f, 1.f };
	m_ReachedKeyframeIndicesThisCommit.clear();
	DiscardEvaluatedStep();
}

UPtr<CComPathPlayback> CComPathPlayback::Create()
{
	auto pInstance = ToUPtr(new CComPathPlayback{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComPathPlayback");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CComPathPlayback::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComPathPlayback{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CComPathPlayback");
		return nullptr;
	}
	return pInstance;
}

void CComPathPlayback::Free()
{
	m_pPathResource.reset();
	CComponent::Free();
}
