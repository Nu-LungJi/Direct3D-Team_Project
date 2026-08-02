#include "pch.h"
#include "ComPathPlayback.h"

#include "DbgLineRender.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "PathPlaybackEvaluator.h"
#include "ResPathPlayback.h"

NS_USING(Engine)

static constexpr _float COM_PATH_PLAYBACK_EPSILON = 0.0001f;
static constexpr uint32_t COM_PATH_PLAYBACK_MAX_BOUNDARY_CROSSINGS = 1024u;

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

_bool CComPathPlayback::Seek(_float fElapsedTime)
{
	const auto* pClip = GetCurrentClip();
	if (!pClip)
		return false;

	m_fElapsedTime = std::clamp(fElapsedTime, 0.f, GetDuration());
	m_ReachedKeyframeIndicesThisCommit.clear();
	DiscardEvaluatedStep();
	if (m_eState == PATH_PLAYBACK_STATE::COMPLETED ||
		m_eState == PATH_PLAYBACK_STATE::INTERRUPTED)
	{
		m_eState = PATH_PLAYBACK_STATE::PAUSED;
	}
	return EvaluatePose(*pClip, m_fElapsedTime, m_tCurrentPose);
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
	ImGui::Text(
		"Clip: %s",
		m_sCurrentClipID.hash != 0
		? m_sCurrentClipID.GetDbgStr()
		: "<none>");
	ImGui::Text("Time: %.3f / %.3f", m_fElapsedTime, GetDuration());
	ImGui::Text("Normalized: %.3f", GetNormalizedTime());
	ImGui::Text("Direction: %s", magic_enum::enum_name(m_eDirection).data());
	ImGui::Text("Pending Evaluation: %s",
		m_bHasPendingEvaluation ? "true" : "false");
	ImGui::Text("Reached This Commit: %zu",
		m_ReachedKeyframeIndicesThisCommit.size());

	if (!ImGui::CollapsingHeader(
		"Path Viewer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}

	if (!m_pPathResource)
	{
		ImGui::TextDisabled("No PathPlayback resource is assigned.");
		return;
	}

	const auto& Clips = m_pPathResource->GetData().Clips;
	if (m_iDebugViewerClipIndex >= static_cast<int32_t>(Clips.size()))
		m_iDebugViewerClipIndex = -1;

	const char* pPreviewClip = "Runtime Current";
	if (m_iDebugViewerClipIndex >= 0)
		pPreviewClip = Clips[m_iDebugViewerClipIndex].sClipID.GetDbgStr();
	if (ImGui::BeginCombo("Viewer Clip", pPreviewClip))
	{
		if (ImGui::Selectable(
			"Runtime Current", m_iDebugViewerClipIndex == -1))
		{
			m_iDebugViewerClipIndex = -1;
		}
		for (size_t i = 0; i < Clips.size(); ++i)
		{
			const _bool bSelected =
				m_iDebugViewerClipIndex == static_cast<int32_t>(i);
			if (ImGui::Selectable(Clips[i].sClipID.GetDbgStr(), bSelected))
				m_iDebugViewerClipIndex = static_cast<int32_t>(i);
			if (bSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	const auto ApplyCurrentPoseToOwner = [this]()
	{
		if (!m_pGameObject)
			return;
		m_pGameObject->GetTransform().SetPosition(m_tCurrentPose.vPosition);
		m_pGameObject->GetTransform().SetQuaternion(m_tCurrentPose.vRotation);
		m_pGameObject->GetTransform().Update();
	};

	const auto* pControlClip = GetDebugViewerClip();
	if (ImGui::Button("Play Forward") && pControlClip &&
		Play(
			pControlClip->sClipID,
			PATH_PLAYBACK_DIRECTION::FORWARD,
			true))
	{
		m_iDebugViewerClipIndex = -1;
		ApplyCurrentPoseToOwner();
	}
	ImGui::SameLine();
	if (ImGui::Button("Play Reverse") && pControlClip &&
		Play(
			pControlClip->sClipID,
			PATH_PLAYBACK_DIRECTION::REVERSE,
			true))
	{
		m_iDebugViewerClipIndex = -1;
		ApplyCurrentPoseToOwner();
	}
	ImGui::SameLine();
	if (ImGui::Button("Pause"))
		Pause();
	ImGui::SameLine();
	if (ImGui::Button("Resume"))
		Resume();
	ImGui::SameLine();
	if (ImGui::Button("Stop"))
		Stop();

	_float fPlaybackRate = m_fPlaybackRate;
	if (ImGui::DragFloat(
		"Playback Rate", &fPlaybackRate, 0.05f, 0.f, 10.f, "%.2f"))
	{
		SetPlaybackRate(fPlaybackRate);
	}

	const _float fRuntimeDuration = GetDuration();
	if (GetCurrentClip() && fRuntimeDuration > COM_PATH_PLAYBACK_EPSILON)
	{
		_float fSeekTime = m_fElapsedTime;
		if (ImGui::SliderFloat(
			"Runtime Time", &fSeekTime,
			0.f, fRuntimeDuration, "%.3f"))
		{
			Pause();
			if (Seek(fSeekTime))
				ApplyCurrentPoseToOwner();
		}
	}

	ImGui::Checkbox("Show Runtime Path", &m_bDebugShowPath);
	ImGui::SameLine();
	ImGui::Checkbox("Path Depth Test", &m_bDebugDepthTest);
	ImGui::Checkbox("Show Keyframes", &m_bDebugShowKeyframes);
	ImGui::SameLine();
	ImGui::Checkbox("Show Current Pose", &m_bDebugShowCurrentPose);
	ImGui::SameLine();
	ImGui::Checkbox("Show Pending Pose", &m_bDebugShowPendingPose);
	ImGui::SliderInt(
		"Debug Lines Per Key Span",
		&m_iDebugLinesPerKeySpan, 2, 32);

	const auto* pViewerClip = GetDebugViewerClip();
	if (!pViewerClip)
	{
		ImGui::TextDisabled(
			m_iDebugViewerClipIndex < 0
			? "No clip is currently playing. Select a resource clip to preview."
			: "The selected clip is unavailable.");
		return;
	}

	ImGui::Text("Keyframes: %zu", pViewerClip->Keyframes.size());
	if (pViewerClip->Keyframes.size() >= 2)
	{
		ImGui::Text(
			"Viewer Duration: %.3f",
			pViewerClip->Keyframes.back().fTime -
			pViewerClip->Keyframes.front().fTime);
	}

	if (m_bDebugShowPath)
	{
		DrawDebugPathViewer(
			*pViewerClip,
			GetDebugViewerAnchor(*pViewerClip));
	}
}

const PATH_PLAYBACK_CLIP* CComPathPlayback::GetDebugViewerClip() const
{
	if (!m_pPathResource)
		return nullptr;
	if (m_iDebugViewerClipIndex < 0)
		return GetCurrentClip();

	const auto& Clips = m_pPathResource->GetData().Clips;
	return static_cast<size_t>(m_iDebugViewerClipIndex) < Clips.size()
		? &Clips[m_iDebugViewerClipIndex]
		: nullptr;
}

PATH_PLAYBACK_POSE CComPathPlayback::GetDebugViewerAnchor(
	const PATH_PLAYBACK_CLIP& Clip) const
{
	if (&Clip == GetCurrentClip())
		return m_tStartAnchorPose;

	PATH_PLAYBACK_POSE Anchor{};
	if (m_pGameObject)
	{
		Anchor.vPosition = m_pGameObject->GetTransform().GetPosition();
		Anchor.vRotation = m_pGameObject->GetTransform().GetQuaternion();
	}
	return Anchor;
}

void CComPathPlayback::DrawDebugPathViewer(
	const PATH_PLAYBACK_CLIP& Clip,
	const PATH_PLAYBACK_POSE& AnchorPose) const
{
	if (Clip.Keyframes.size() < 2)
		return;
	auto* pDebug = CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const _float4 PreviousColor = pDebug->GetColor();
	const DBG_LINE_DEPTH_MODE PreviousDepth = pDebug->GetDepthMode();
	pDebug->SetDepthTest(m_bDebugDepthTest);

	const _float fDuration = Clip.Keyframes.back().fTime -
		Clip.Keyframes.front().fTime;
	const int32_t iSampleCount = std::max(
		2,
		static_cast<int32_t>(Clip.Keyframes.size() - 1) *
			std::max(2, m_iDebugLinesPerKeySpan));

	PATH_PLAYBACK_POSE PreviousPose{};
	_bool bHasPrevious{};
	for (int32_t i = 0; i <= iSampleCount; ++i)
	{
		const _float fTime = fDuration * static_cast<_float>(i) /
			static_cast<_float>(iSampleCount);
		PATH_PLAYBACK_POSE Pose{};
		CPathPlaybackEvaluator::CONTEXT Context{};
		Context.tStartAnchorPose = AnchorPose;
		Context.tCurrentObjectPose = PreviousPose;
		Context.bHasCurrentObjectPose = bHasPrevious;
		if (!CPathPlaybackEvaluator::EvaluatePose(
			Clip, fTime, Context, Pose))
		{
			continue;
		}
		if (bHasPrevious)
		{
			pDebug->AddLine(
				PreviousPose.vPosition,
				Pose.vPosition,
				{ 0.1f, 0.8f, 1.f, 1.f });
		}
		PreviousPose = Pose;
		bHasPrevious = true;
	}

	if (m_bDebugShowKeyframes)
	{
		pDebug->SetColor({ 0.25f, 1.f, 0.35f, 1.f });
		for (const auto& Keyframe : Clip.Keyframes)
		{
			PATH_PLAYBACK_POSE KeyPose{};
			CPathPlaybackEvaluator::CONTEXT Context{};
			Context.tStartAnchorPose = AnchorPose;
			const _float fKeyTime =
				Keyframe.fTime - Clip.Keyframes.front().fTime;
			if (CPathPlaybackEvaluator::EvaluatePose(
				Clip, fKeyTime, Context, KeyPose))
			{
				pDebug->AddCross(KeyPose.vPosition, 0.15f);
			}
		}
	}

	const _bool bViewingRuntimeClip = &Clip == GetCurrentClip();
	PATH_PLAYBACK_POSE ViewerPose{};
	_bool bHasViewerPose{};
	if (bViewingRuntimeClip &&
		m_eState != PATH_PLAYBACK_STATE::IDLE)
	{
		ViewerPose = m_tCurrentPose;
		bHasViewerPose = true;
	}
	else
	{
		CPathPlaybackEvaluator::CONTEXT Context{};
		Context.tStartAnchorPose = AnchorPose;
		bHasViewerPose = CPathPlaybackEvaluator::EvaluatePose(
			Clip, 0.f, Context, ViewerPose);
	}

	if (m_bDebugShowCurrentPose && bHasViewerPose)
	{
		pDebug->SetColor({ 1.f, 0.85f, 0.1f, 1.f });
		pDebug->AddSphere(
			0.18f,
			XMMatrixTranslationFromVector(
				XMLoadFloat3(&ViewerPose.vPosition)));
	}

	if (m_bDebugShowPendingPose &&
		bViewingRuntimeClip && m_bHasPendingEvaluation)
	{
		pDebug->SetColor({ 1.f, 0.15f, 0.9f, 1.f });
		pDebug->AddSphere(
			0.14f,
			XMMatrixTranslationFromVector(
				XMLoadFloat3(&m_tPendingStep.tTargetPose.vPosition)));
		if (bHasViewerPose)
		{
			pDebug->AddLine(
				ViewerPose.vPosition,
				m_tPendingStep.tTargetPose.vPosition,
				{ 1.f, 0.15f, 0.9f, 1.f });
		}
	}

	pDebug->SetColor(PreviousColor);
	pDebug->SetDepthMode(PreviousDepth);
}

_bool CComPathPlayback::EvaluatePose(
	const PATH_PLAYBACK_CLIP& Clip,
	_float fElapsedTime,
	PATH_PLAYBACK_POSE& OutPose,
	size_t* pOutSegmentIndex) const
{
	CPathPlaybackEvaluator::CONTEXT Context{};
	Context.tStartAnchorPose = m_tStartAnchorPose;
	if (m_pGameObject)
	{
		Context.tCurrentObjectPose.vPosition =
			m_pGameObject->GetTransform().GetPosition();
		Context.tCurrentObjectPose.vRotation =
			m_pGameObject->GetTransform().GetQuaternion();
		Context.bHasCurrentObjectPose = true;
	}

	return CPathPlaybackEvaluator::EvaluatePose(
		Clip,
		fElapsedTime,
		Context,
		OutPose,
		pOutSegmentIndex);
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
