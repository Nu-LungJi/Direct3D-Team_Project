#pragma once

#include "AmbientSoundData.h"
#include "GameObject.h"

NS_BEGIN(Engine)

class CComSound;

class ENGINE_DLL CAmbientSound2DObject final : public CGameObject
{
public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		AMBIENT_SOUND_2D_DATA tSoundData{};
	};

public:
	DECLARE_DERIVED_TYPE(CAmbientSound2DObject, CGameObject)

private:
	CAmbientSound2DObject() = default;
	CAmbientSound2DObject(const CAmbientSound2DObject&) = default;
	~CAmbientSound2DObject() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void Update(_float fTimeDelta) override;
	void UpdateGUI() override;

public:
	SOUND_ID Play();
	_bool Stop();
	_bool FadeOutAndStop();
	_bool FadeOutAndDetach();
	_bool ApplyData(const AMBIENT_SOUND_2D_DATA& tData);
	void SetEnabled(_bool bEnabled);

	_bool IsPlaying() const;
	_bool IsEnabled() const { return m_tSoundData.bEnabled; }
	const AMBIENT_SOUND_2D_DATA& GetData() const { return m_tSoundData; }

public:
	static UPtr<CAmbientSound2DObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	static const StringID& GetAmbientSlotID();
	static void SanitizeData(AMBIENT_SOUND_2D_DATA& tData);

private:
	CComSound* m_pComSound{};
	AMBIENT_SOUND_2D_DATA m_tSoundData{};
	SOUND_ID m_iSoundID{ INVALID_SOUND_ID };

private:
	void Free() override;
};

NS_END
