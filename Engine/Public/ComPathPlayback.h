#pragma once

#include "Component.h"
#include "PathPlaybackDefines.h"

NS_BEGIN(Engine)

class CResPathPlayback;

class ENGINE_DLL CComPathPlayback final : public CComponent
{
public:
	DECLARE_DERIVED_TYPE(CComPathPlayback, CComponent)

	struct DESC : public CComponent::DESC
	{
		SPtr<CResPathPlayback> pPathResource{};
		_float fPlaybackRate{ 1.f };
	};

private:
	struct CURSOR_RESULT
	{
		_float fElapsedTime{};
		PATH_PLAYBACK_DIRECTION eDirection{
			PATH_PLAYBACK_DIRECTION::FORWARD };
		PATH_PLAYBACK_STATE eState{ PATH_PLAYBACK_STATE::PLAYING };
		_bool bWrapped{};
	};

private:
	explicit CComPathPlayback();
	explicit CComPathPlayback(const CComPathPlayback& Prototype);
	~CComPathPlayback() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	_bool SetPathResource(SPtr<CResPathPlayback> pPathResource);
	const SPtr<CResPathPlayback>& GetPathResource() const
	{
		return m_pPathResource;
	}

	_bool Play(
		const StringID& sClipID,
		PATH_PLAYBACK_DIRECTION eDirection =
			PATH_PLAYBACK_DIRECTION::FORWARD,
		_bool bRestart = true);
	void Pause();
	void Resume();
	void Stop();
	void Interrupt();

	// 상태를 변경하지 않고 이번 Fixed Tick의 목표 Pose만 계산한다.
	PATH_PLAYBACK_STEP_RESULT EvaluateNext(_float fFixedTimeDelta);
	// 실제 이동에 성공한 비율만큼 재생 시간을 확정한다. 0~1 범위이다.
	_bool CommitEvaluatedStep(_float fAcceptedRatio = 1.f);
	void DiscardEvaluatedStep();

public:
	PATH_PLAYBACK_STATE GetState() const { return m_eState; }
	_bool IsPlaying() const { return m_eState == PATH_PLAYBACK_STATE::PLAYING; }
	_bool IsCompleted() const { return m_eState == PATH_PLAYBACK_STATE::COMPLETED; }
	_bool IsInterrupted() const { return m_eState == PATH_PLAYBACK_STATE::INTERRUPTED; }
	_bool HasPendingEvaluation() const { return m_bHasPendingEvaluation; }

	const StringID& GetCurrentClipID() const { return m_sCurrentClipID; }
	const PATH_PLAYBACK_CLIP* GetCurrentClip() const;
	const PATH_PLAYBACK_POSE& GetCurrentPose() const { return m_tCurrentPose; }
	_float GetElapsedTime() const { return m_fElapsedTime; }
	_float GetDuration() const;
	_float GetNormalizedTime() const;
	PATH_PLAYBACK_DIRECTION GetDirection() const { return m_eDirection; }
	void SetPlaybackRate(_float fPlaybackRate);
	_float GetPlaybackRate() const { return m_fPlaybackRate; }

	const std::vector<size_t>& GetReachedKeyframeIndicesThisCommit() const
	{
		return m_ReachedKeyframeIndicesThisCommit;
	}
	const PATH_PLAYBACK_KEYFRAME* GetKeyframe(size_t iKeyframeIndex) const;

public:
	void UpdateGUI() override;

private:
	_bool EvaluatePose(
		const PATH_PLAYBACK_CLIP& Clip,
		_float fElapsedTime,
		PATH_PLAYBACK_POSE& OutPose,
		size_t* pOutSegmentIndex = nullptr) const;
	CURSOR_RESULT AdvanceCursor(
		const PATH_PLAYBACK_CLIP& Clip,
		_float fDeltaTime,
		_bool bCollectReachedKeyframes);
	void CollectReachedKeyframes(
		const PATH_PLAYBACK_CLIP& Clip,
		_float fStartElapsed,
		_float fEndElapsed,
		PATH_PLAYBACK_DIRECTION eDirection);
	void ResetRuntimeState();

private:
	SPtr<CResPathPlayback> m_pPathResource{};
	StringID m_sCurrentClipID{};
	PATH_PLAYBACK_STATE m_eState{ PATH_PLAYBACK_STATE::IDLE };
	PATH_PLAYBACK_DIRECTION m_eDirection{
		PATH_PLAYBACK_DIRECTION::FORWARD };
	PATH_PLAYBACK_DIRECTION m_eInitialDirection{
		PATH_PLAYBACK_DIRECTION::FORWARD };

	_float m_fElapsedTime{};
	_float m_fPlaybackRate{ 1.f };
	PATH_PLAYBACK_POSE m_tStartAnchorPose{};
	PATH_PLAYBACK_POSE m_tCurrentPose{};

	PATH_PLAYBACK_STEP_RESULT m_tPendingStep{};
	_float m_fPendingAdvanceTime{};
	_bool m_bHasPendingEvaluation{};
	std::vector<size_t> m_ReachedKeyframeIndicesThisCommit{};

public:
	static UPtr<CComPathPlayback> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
