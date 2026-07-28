#pragma once

#include "Component.h"
#include <numbers>

NS_BEGIN(Engine)

enum class EEaseType { Linear, EaseOutQuad, EaseOutBack, EaseOutElastic, EaseOutBounce, Floating, EaseInElastic };

namespace Easing
{
	// 부드럽게 감속
	inline float EaseOutQuad(float t) {
		return 1.0f - (1.0f - t) * (1.0f - t);
	}
	inline float Floating(float t) {
		return -(std::cos(std::numbers::pi_v<float> *t) - 1.0f) * 2.0f;
	}


	// 오버슛
	inline float EaseOutBack(float t) {
		const float c1 = 1.70158f;
		const float c3 = c1 + 1.0f;
		return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
	}

	// 3. 탄성
	inline float EaseOutElastic(float t) {
		const float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
		if (t == 0.0f) return 0.0f;
		if (t == 1.0f) return 1.0f;
		return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
	}

	// 4. 바운스
	inline float EaseOutBounce(float t) {
		const float n1 = 7.5625f;
		const float d1 = 2.75f;
		if (t < 1.0f / d1) {
			return n1 * t * t;
		}
		else if (t < 2.0f / d1) {
			return n1 * (t -= 1.5f / d1) * t + 0.75f;
		}
		else if (t < 2.5f / d1) {
			return n1 * (t -= 2.25f / d1) * t + 0.9375f;
		}
		else {
			return n1 * (t -= 2.625f / d1) * t + 0.984375f;
		}
	}

	inline float EaseInElastic(float t) {
		const float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
		if (t == 0.0f) return 0.0f;
		if (t == 1.0f) return 1.0f;
		return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
	}
}

struct FUITween
{
	float fStartValue;
	float fEndValue;
	float fDuration = 1.f;
	float fCurrentTime = 0.0f;

	std::function<void(float)> OnUpdate;

	std::function<void()> OnComplete;

	EEaseType eEaseType = EEaseType::Linear;
	float fDelay = 0.f;          // 대기 시간
	float fDelayTimer = 0.f;     // 대기 시간 측정용 타이머
	bool bPingPong = false;      // 반복 여부

	bool bIsFinished = false;
};

class ENGINE_DLL TweenComponent : public CComponent
{
public:
	DECLARE_DERIVED_TYPE(TweenComponent, CComponent)
private:
	explicit TweenComponent();
	~TweenComponent() override;


private:
	std::vector<FUITween> m_ActiveTweens;
public:
	void PlayTween(float start, float end, float duration, std::function<void(float)> onUpdate, std::function<void()> onComplete = nullptr, 
		EEaseType eEaseType = EEaseType::Linear, float delay = 0.f, bool pingPong = false);
	void Tick(_float fTimeDelta);

	void ClearTweens();

public:
	virtual HRESULT		Initialize(void* pArg) override;

public:
	static UPtr<TweenComponent> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
