#include "pch.h"

#include "TweenComponent.h"
#include "UIObject.h"

NS_USING(Engine)

TweenComponent::TweenComponent()
{
}

TweenComponent::~TweenComponent()
{
}

void TweenComponent::PlayTween(float start, float end, float duration, std::function<void(float)> onUpdate, std::function<void()> onComplete
	, EEaseType eEaseType, float delay, bool pingPong)
{
	FUITween newTween;
	newTween.fStartValue = start;
	newTween.fEndValue = end;
	newTween.fDuration = duration;
	newTween.OnUpdate = onUpdate;
	newTween.OnComplete = onComplete;
	newTween.eEaseType = eEaseType;
	newTween.fDelay = delay;
	newTween.bPingPong = pingPong;

	m_ActiveTweens.push_back(newTween);
}

void TweenComponent::Tick(_float fTimeDelta)
{
	for (auto& tween : m_ActiveTweens)
	{
		if (tween.bIsFinished) continue;

		// --------------------------------------------------
		// 1. 딜레이(Delay) 처리: 대기 시간이 안 끝났으면 업데이트 스킵
		// --------------------------------------------------
		if (tween.fDelayTimer < tween.fDelay)
		{
			tween.fDelayTimer += fTimeDelta;
			continue;
		}

		// --------------------------------------------------
		// 2. 시간 진행 및 이징(Easing) 계산
		// --------------------------------------------------
		tween.fCurrentTime += fTimeDelta;
		float fRatio = std::clamp(tween.fCurrentTime / tween.fDuration, 0.0f, 1.0f);

		float fEasedRatio = fRatio;
		switch (tween.eEaseType)
		{
		case EEaseType::EaseOutQuad:    fEasedRatio = Easing::EaseOutQuad(fRatio); break;
		case EEaseType::EaseOutBack:    fEasedRatio = Easing::EaseOutBack(fRatio); break;
		case EEaseType::EaseOutElastic: fEasedRatio = Easing::EaseOutElastic(fRatio); break;
		case EEaseType::EaseOutBounce:  fEasedRatio = Easing::EaseOutBounce(fRatio); break;
		case EEaseType::Linear: default: break;
		}

		// --------------------------------------------------
		// 3. 값 보간 및 Update 콜백 실행
		// --------------------------------------------------
		float fCurrentValue = std::lerp(tween.fStartValue, tween.fEndValue, fEasedRatio);

		if (tween.OnUpdate)
			tween.OnUpdate(fCurrentValue);

		// --------------------------------------------------
		// 4. 완료 및 핑퐁(PingPong) 처리
		// --------------------------------------------------
		if (fRatio >= 1.0f)
		{
			if (tween.bPingPong)
			{
				std::swap(tween.fStartValue, tween.fEndValue);
				tween.fCurrentTime = 0.0f;
			}
			else
			{
				tween.bIsFinished = true;
				if (tween.OnComplete)
					tween.OnComplete();
			}
		}
	}

	// 완료된 트윈 삭제
	std::erase_if(m_ActiveTweens, [](const FUITween& t) { return t.bIsFinished; });
}

void TweenComponent::ClearTweens()
{
	m_ActiveTweens.clear();
}

HRESULT TweenComponent::Initialize(void* pArg)
{
	if (FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

UPtr<TweenComponent> TweenComponent::Create()
{
	auto pInstance = ToUPtr(new TweenComponent{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : TweenComponent");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> TweenComponent::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new TweenComponent{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: TweenComponent");
		return nullptr;
	}
	return pInstance;
}
