#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class CComModelInstance;
class CComAnimator;

class ENGINE_DLL CComAnimMontage  : public CComponent
{
public:
	enum class EVENT_TYPE : uint32_t {
		SOUND,
		PARTICLE,
		CALLBACK_END
	};

	struct MONTAGE_CLIP
	{
		std::string strAnimName;
		uint32_t    iAnimIndex = 0;

		float fMontageStartTime = 0.f;

		float fAnimStartTime = 0.f;
		float fAnimEndTime = 0.f;

		float fPlayRate = 1.f;

		float fBlendInTime = 0.f;
		float fBlendOutTime = 0.f;
	};

	struct MONTAGE_EVENT
	{
		float fTime = 0.f;

		EVENT_TYPE eType = EVENT_TYPE::CALLBACK_END;

		std::string strName;
		std::string strPayload;
		std::string strBoneName;
	};
	public:
		CComAnimMontage();
		
		virtual ~CComAnimMontage();

	public:
		virtual HRESULT Initialize(void* pArg) override;
		virtual HRESULT Update(_float fTimeDelta) ;

	public:
		HRESULT Play();
		HRESULT Stop();
		HRESULT Reset();

	public:
		HRESULT Load_Montage(const std::string& strPath);
		HRESULT Save_Montage(const std::string& strPath);

	public:
		void AddClip(const MONTAGE_CLIP& clip);
		void AddEvent(const MONTAGE_EVENT& eventDesc);

	public:
		_bool IsPlaying() const {
			return m_bPlaying;
		}

		float GetCurrentTime() const {
			return m_fCurrentTime;
		}

		float GetDuration() const {
			return m_fDuration;
		}

	private:
		HRESULT Update_Montage(_float fTimeDelta);
		HRESULT Process_Clips();
		HRESULT Process_Events();

	private:
		_bool IsClipActive(const MONTAGE_CLIP& clip, float fMontageTime);
		float CalcClipLocalTime(const MONTAGE_CLIP& clip, float fMontageTime);
		float CalcClipWeight(const MONTAGE_CLIP& clip, float fMontageTime);

	private:
		void DispatchEvent(const MONTAGE_EVENT& eventDesc);
		void PlaySoundEvent(const MONTAGE_EVENT& eventDesc);
		void SpawnParticleEvent(const MONTAGE_EVENT& eventDesc);
		void ExecuteCallbackEvent(const MONTAGE_EVENT& eventDesc);

	public:
		static UPtr<CComAnimMontage> Create();
		virtual UPtr<CPrototype> Clone(void* pArg) override;

	private:
		CComModelInstance* m_pModelInstance = nullptr;
		CComAnimator* m_pAnimator = nullptr;

	private:
		std::vector<MONTAGE_CLIP>  m_Clips;
		std::vector<MONTAGE_EVENT> m_Events;

	private:
		float m_fDuration = 0.f;

		float m_fCurrentTime = 0.f;
		float m_fPrevTime = 0.f;
		float m_fPlayRate = 1.f;

		_bool m_bPlaying = false;
		_bool m_bLoop = false;

		std::vector<_bool> m_EventPlayed;
};


NS_END
