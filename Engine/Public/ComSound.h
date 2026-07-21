#pragma once

#include "Component.h"
#include "SoundManager.h"

NS_BEGIN(Engine)

enum class SOUND_SLOT_PLAY_MODE : uint8_t
{
	REPLACE,
	OVERLAP
};

class ENGINE_DLL CComSound final : public CComponent
{
public:
	struct DESC : public CComponent::DESC
	{
	};

public:
	DECLARE_DERIVED_TYPE(CComSound, CComponent)

private:
	explicit CComSound();
	CComSound(const CComSound& rhs);
	~CComSound() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	// 반환 ID는 확장용이며 수명과 소유권은 계속 컴포넌트가 관리한다.
	SOUND_ID Play2D(const _string& sPath, const SOUND_PLAY_DESC& tPlayDesc,
		SOUND_LOAD_TYPE eLoadType = SOUND_LOAD_TYPE::SAMPLE);

	SOUND_ID PlaySlot2D(const StringID& sSlotID, const _string& sPath,
		const SOUND_PLAY_DESC& tPlayDesc,
		SOUND_SLOT_PLAY_MODE ePlayMode = SOUND_SLOT_PLAY_MODE::REPLACE,
		SOUND_LOAD_TYPE eLoadType = SOUND_LOAD_TYPE::SAMPLE);

	SOUND_ID Play3D(const _string& sPath, const SOUND_3D_DESC& t3DDesc,
		const SOUND_PLAY_DESC& tPlayDesc,
		SOUND_LOAD_TYPE eLoadType = SOUND_LOAD_TYPE::SAMPLE);

	// 이후 제어가 필요한 사운드는 슬롯 이름으로 접근한다.
	SOUND_ID PlaySlot3D(const StringID& sSlotID, const _string& sPath,
		const SOUND_3D_DESC& t3DDesc, const SOUND_PLAY_DESC& tPlayDesc,
		SOUND_SLOT_PLAY_MODE ePlayMode = SOUND_SLOT_PLAY_MODE::REPLACE,
		SOUND_LOAD_TYPE eLoadType = SOUND_LOAD_TYPE::SAMPLE);

	_bool StopSlot(const StringID& sSlotID);
	void StopAll();
	_bool SetSlotPaused(const StringID& sSlotID, _bool bPaused);
	_bool SetSlotVolume(const StringID& sSlotID, _float fVolume);
	_bool SetSlotPitch(const StringID& sSlotID, _float fPitch);
	_bool SetSlot3DAttributes(const StringID& sSlotID, const _float3& vPosition,
		const _float3& vVelocity = {});
	_bool SetSlot3DMinMaxDistance(const StringID& sSlotID,
		_float fMinDistance, _float fMaxDistance);
	_bool IsValidSlot(const StringID& sSlotID) const;

	// 자연 종료된 내부 SOUND_ID와 슬롯 참조를 정리한다.
	void Update();

public:
	void UpdateGUI() override;

private:
	SOUND_ID PlayInternal(const _string& sPath,
		const SOUND_PLAY_DESC& tPlayDesc, SOUND_LOAD_TYPE eLoadType);
	SOUND_ID PlayInternal(const _string& sPath, const SOUND_3D_DESC& t3DDesc,
		const SOUND_PLAY_DESC& tPlayDesc, SOUND_LOAD_TYPE eLoadType);
	_bool StopInternal(SOUND_ID iSoundID);
	void RemoveSlotReference(SOUND_ID iSoundID);

private:
	std::unordered_set<SOUND_ID> m_PlayingSounds{};
	std::unordered_map<StringID, std::vector<SOUND_ID>> m_Slots{};

public:
	static UPtr<CComSound> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
