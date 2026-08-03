#pragma once
#include "Client_Defines.h"
#include "Level.h"
#include "UIObject.h"
NS_BEGIN(Client)

class CLevelLogo final : public Engine::CLevel
{
public:
	DECLARE_DERIVED_TYPE(CLevelLogo, CLevel)

private:
	explicit CLevelLogo();
	~CLevelLogo() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;
	void FrameStart(E::_float fTimeDelta) override;

private:
	_bool m_VideoQue{ false };
	_bool m_ChangeScene{ false };
	_bool m_isLogoDelete{ false };
	CHandle m_Video{};
	CHandle m_Logo{};

public:
	static Engine::UPtr<CLevelLogo> Create();

private:
	CUIObject* SafeGetOBJ(CHandle pHandle)
	{
		if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle))
			return E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle);
	}
	void PlayFadeOutDelete(CHandle pHandle, float delay = 1.f, float playtime = 5.f);
	void PlayFadeIn(CHandle pHandle, float delay = 0.f, float playtime = 5.f);
	void PlayFadeInChange(CHandle pHandle, float delay = 0.f, float playtime = 3.f);

	_float m_SceneChangeTimer = 0.f;

private:
	void Free() override;
};

NS_END
