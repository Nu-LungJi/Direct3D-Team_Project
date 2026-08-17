#include "pch.h"
#include "SpellMiniGame.h"

#include "GameInstance.h"
#include "EffectUI.h"
#include "Level_Defines.h"
#include "SpellMeter.h"
#include "TextureUI.h"
#include "UI_Enums.h"
#include "UIManager.h"

NS_USING(Client)

namespace
{
	_float Length(const _float2& value)
	{
		return sqrtf(value.x * value.x + value.y * value.y);
	}

	_float2 Normalize(const _float2& value)
	{
		const _float length = Length(value);
		if (length <= FLT_EPSILON)
			return {};

		return { value.x / length, value.y / length };
	}

	_float Dot(const _float2& lhs, const _float2& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y;
	}

	_float2 Lerp(
		const _float2& start,
		const _float2& end,
		_float ratio)
	{
		return {
			std::lerp(start.x, end.x, ratio),
			std::lerp(start.y, end.y, ratio)
		};
	}
}

CSpellMiniGame::CSpellMiniGame() = default;

CSpellMiniGame::CSpellMiniGame(const CSpellMiniGame& rhs)
	: CGameObject{ rhs }
{
}

HRESULT CSpellMiniGame::InitializePrototype(void*)
{
	return S_OK;
}

HRESULT CSpellMiniGame::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	const _float2 clientSize =
		E::CGameInstance::Get().GetClientScreenSize();
	m_fPathSize = std::min(clientSize.x, clientSize.y) * 0.82f;
	m_vPathTopLeft = {
		(clientSize.x - m_fPathSize) * 0.5f,
		(clientSize.y - m_fPathSize) * 0.5f
	};

	BuildIncendioPath();
	if (m_PathSamples.size() < 2 || !CreateVisuals())
		return E_FAIL;

	ResetToStart();
	return S_OK;
}

void CSpellMiniGame::PriorityUpdate(_float)
{
}

void CSpellMiniGame::Update(_float fTimeDelta)
{
	if (m_PathSamples.size() < 2)
		return;

	const _float safeDelta = std::min(fTimeDelta, 0.05f);
	const _float movementDelta = std::min(
		fTimeDelta,
		MAX_MOVEMENT_DELTA);
	UpdateTransientEffects(safeDelta);
	UpdateBoostTrailParticles(safeDelta);
	if (m_eState == STATE::INTRO)
	{
		UpdateIntro(fTimeDelta);
		return;
	}
	if (m_eState == STATE::COMPLETED)
	{
		UpdateCompletionSequence(std::max(0.f, fTimeDelta));
		return;
	}

	if (m_fBoostTimeRemaining > 0.f)
	{
		m_fBoostTimeRemaining = std::max(
			0.f,
			m_fBoostTimeRemaining - safeDelta);
	}

	const _bool aPressed =
		E::CGameInstance::Get().KeyDown(DIK_A);
	const _bool xPressed =
		E::CGameInstance::Get().KeyDown(DIK_X);
	if (m_eState == STATE::WAITING && aPressed)
	{
		m_eState = STATE::RUNNING;
		ActivateBoost();
		PlayMagicBurst(m_StartPadSuccessEffect, m_hStartPad);
	}
	if (m_eState == STATE::RUNNING)
		TryActivateBoostPad(aPressed, xPressed);

	const _float2 currentPosition =
		EvaluatePosition(m_fPathDistance);
	_float2 facingDirection = m_vLastFacingDirection;

	const _float2 mousePosition =
		E::CGameInstance::Get().GetMousePos();
	const _float2 toMouse = {
		mousePosition.x - currentPosition.x,
		mousePosition.y - currentPosition.y
	};
	const _float mouseDistance = Length(toMouse);
	if (mouseDistance > MOUSE_DIRECTION_MIN_DISTANCE)
	{
		facingDirection = Normalize(toMouse);
		m_vLastFacingDirection = facingDirection;
	}

	if (m_eState == STATE::RUNNING &&
		mouseDistance > MOUSE_DIRECTION_MIN_DISTANCE)
	{
		const _float2 pathForward =
			EvaluateForward(m_fPathDistance);
		const _float rawAlignment = std::clamp(
			Dot(pathForward, facingDirection),
			-1.f,
			1.f);

		const _float absoluteAlignment =
			fabsf(rawAlignment);
		if (absoluteAlignment > ALIGNMENT_DEAD_ZONE)
		{
			const _float baseSpeed = MAX_MOVE_SPEED;
			_float boostRatio = std::clamp(
				m_fBoostTimeRemaining / BOOST_DURATION,
				0.f,
				1.f);
			boostRatio = boostRatio * boostRatio *
				(3.f - 2.f * boostRatio);
			const _float speed = baseSpeed * std::lerp(
				1.f,
				BOOST_MAX_MULTIPLIER,
				boostRatio);
			const _float direction =
				rawAlignment >= 0.f ? 1.f : -1.f;

			m_fPathDistance = std::clamp(
				m_fPathDistance + direction * speed * movementDelta,
				0.f,
				m_fTotalPathDistance);
		}

		if (m_fPathDistance >= m_fTotalPathDistance - 0.5f)
		{
			m_fPathDistance = m_fTotalPathDistance;
			m_eState = STATE::COMPLETED;
			E::CGameInstance::Get().GetSoundManager()->Play2D(
				"./Resources/SampleClient/Sound/UI/MiniGameSucces.wav",
				SOUND_PLAY_DESC{
					.sBusID = SOUND_BUS::UI,
					.fVolume = 1.f,
					.fPitch = 1.f,
					.iPriority = 64,
					.bLoop = false
				});
			PlayDestinationSuccessFlame();
			PlayDestinationSuccessMeterScale();
			PlayDestinationSuccessDiamondPulse();
			m_eCompletionPhase = COMPLETION_PHASE::SUCCESS_ANIMATION;
			m_fCompletionPhaseElapsed = 0.f;
			ResetChaser();
			ResetBoostCursorRipple();
			SetCursorVisible(false);
			SetStartPadVisible(true);
		}
	}

	UpdateChaser(movementDelta, fTimeDelta);

	UpdateArrowVisual(
		EvaluatePosition(m_fPathDistance),
		facingDirection);
	UpdateBoostCursorRipple(safeDelta);
	UpdateBoostTrailEmitter(
		safeDelta,
		EvaluatePosition(m_fPathDistance),
		EvaluateForward(m_fPathDistance));
	if (auto* pathProgress = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hPathProgress))
	{
		pathProgress->SetPathProgress(GetProgress());
	}
	if (auto* spellMeter = E::CGameInstance::Get().
		GetGameObjectByHandleT<CSpellMeter>(m_hDestinationSpellMeter))
	{
		spellMeter->SetFillAmount(GetProgress());
	}
}

void CSpellMiniGame::LateUpdate(_float)
{
}

HRESULT CSpellMiniGame::Render(
	ID3D11DeviceContext*,
	const E::RENDER_CTX&)
{
	return S_OK;
}

_float CSpellMiniGame::GetProgress() const
{
	if (m_fTotalPathDistance <= FLT_EPSILON)
		return 0.f;

	return m_fPathDistance / m_fTotalPathDistance;
}

void CSpellMiniGame::UpdateIntro(_float fTimeDelta)
{
	m_fIntroElapsed = std::min(
		INTRO_TOTAL_DURATION,
		m_fIntroElapsed + std::max(0.f, fTimeDelta));

	const _float fadeRatio = std::clamp(
		m_fIntroElapsed / INTRO_FADE_DURATION,
		0.f,
		1.f);
	SetIntroAlpha(fadeRatio * fadeRatio * (3.f - 2.f * fadeRatio));

	const _float pathRatio = std::clamp(
		m_fIntroElapsed / INTRO_PATH_DURATION,
		0.f,
		1.f);
	if (auto* introPath = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hIntroPathProgress))
	{
		introPath->SetPathProgress(pathRatio);
	}
	UpdateIntroPadScales(m_fIntroElapsed);

	if (m_fIntroElapsed >= INTRO_PATH_DURATION)
	{
		if (!m_bStartPadIntroRevealed)
		{
			m_bStartPadIntroRevealed = true;
			PlayCreateButtonSound();
			CreateBoostSuccessSmoke(
				m_StartPadSuccessEffect.SmokeHandle,
				m_hStartPad,
				905,
				INTRO_SMOKE_SIZE_SCALE,
				{ 0.05f, 0.24f, 1.f },
				INTRO_SMOKE_DURATION);
		}

		const _float startPadFadeRatio = std::clamp(
			(m_fIntroElapsed - INTRO_PATH_DURATION) /
			INTRO_START_PAD_FADE_DURATION,
			0.f,
			1.f);
		const _float easedFadeRatio = startPadFadeRatio * startPadFadeRatio *
			(3.f - 2.f * startPadFadeRatio);
		if (auto* backdrop = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(m_hStartPadBackdrop))
		{
			backdrop->SetAlpha(easedFadeRatio);
		}
		if (auto* icon = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(m_hStartPad))
		{
			icon->SetAlpha(easedFadeRatio);
		}
	}

	UpdateArrowVisual(
		EvaluatePosition(0.f),
		m_vLastFacingDirection);

	if (m_fIntroElapsed >= INTRO_TOTAL_DURATION)
	{
		SetIntroAlpha(1.f);
		UpdateIntroPadScales(INTRO_TOTAL_DURATION);
		m_eState = STATE::WAITING;
	}
}

void CSpellMiniGame::SetIntroAlpha(_float alpha)
{
	alpha = std::clamp(alpha, 0.f, 1.f);
	const CHandle handles[] = {
		m_hPath,
		m_hIntroPathProgress,
		m_hCursor,
		m_hArrow
	};

	for (const CHandle handle : handles)
	{
		if (auto* ui = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(handle))
		{
			ui->SetAlpha(alpha);
		}
	}

	const _float meterRatio = std::clamp(
		m_fIntroElapsed / INTRO_SPELL_METER_DURATION,
		0.f,
		1.f);
	const _float easedMeterRatio = 1.f -
		(1.f - meterRatio) * (1.f - meterRatio);
	if (auto* spellMeter = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hDestinationSpellMeter))
	{
		const _float size = DESTINATION_SPELL_METER_SIZE * easedMeterRatio;
		spellMeter->SetAlpha(1.f);
		spellMeter->SetSize({ size, size });
		spellMeter->CalcUICoord();
	}
	if (auto* border = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hDestinationSpellMeterBorder))
	{
		const _float size = DESTINATION_SPELL_METER_BORDER_SIZE * easedMeterRatio;
		border->SetAlpha(1.f);
		border->SetSize({ size, size });
		border->CalcUICoord();
	}
}

void CSpellMiniGame::UpdateIntroPadScales(_float elapsedTime)
{
	auto updatePadScale = [this, elapsedTime](
		CHandle backdropHandle,
		CHandle iconHandle,
		CHandle smokeHandle,
		_float revealTime,
		_bool& revealed)
		{
			if (elapsedTime >= revealTime && !revealed)
			{
				revealed = true;
				PlayCreateButtonSound();
				CreateBoostSuccessSmoke(
					smokeHandle,
					iconHandle,
					905,
					INTRO_SMOKE_SIZE_SCALE,
					{ 0.05f, 0.24f, 1.f },
					INTRO_SMOKE_DURATION);
			}

			const _float ratio = std::clamp(
				(elapsedTime - revealTime) / INTRO_PAD_SCALE_DURATION,
				0.f,
				1.f);
			const _float easedRatio = 1.f - (1.f - ratio) * (1.f - ratio);

			if (auto* backdrop = E::CGameInstance::Get().
				GetGameObjectByHandleT<E::CUIObject>(backdropHandle))
			{
				const _float size = BOOST_PAD_BACKDROP_SIZE * easedRatio;
				backdrop->SetSize({ size, size });
				backdrop->CalcUICoord();
			}
			if (auto* icon = E::CGameInstance::Get().
				GetGameObjectByHandleT<E::CUIObject>(iconHandle))
			{
				const _float size = BOOST_PAD_ICON_SIZE * easedRatio;
				icon->SetSize({ size, size });
				icon->CalcUICoord();
			}
		};

	for (BOOST_PAD& pad : m_BoostPads)
	{
		const _float progress = m_fTotalPathDistance > FLT_EPSILON ?
			pad.PathDistance / m_fTotalPathDistance : 0.f;
		updatePadScale(
			pad.BackdropHandle,
			pad.Handle,
			pad.SuccessEffect.SmokeHandle,
			progress * INTRO_PATH_DURATION,
			pad.IntroRevealed);
	}
}

void CSpellMiniGame::PlayCreateButtonSound()
{
	E::CGameInstance::Get().GetSoundManager()->Play2D(
		"./Resources/SampleClient/Sound/UI/CreateButton.wav",
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::UI,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});
}

void CSpellMiniGame::ResetToStart()
{
	m_eState = STATE::INTRO;
	m_eCompletionPhase = COMPLETION_PHASE::NONE;
	m_fCompletionPhaseElapsed = 0.f;
	m_fIntroElapsed = 0.f;
	m_bStartPadIntroRevealed = false;
	m_fPathDistance = 0.f;
	m_fBoostTimeRemaining = 0.f;
	m_vLastFacingDirection = EvaluateForward(0.f);
	ResetChaser();
	ResetBoostPads();
	ClearTransientEffects();
	ClearBoostTrailParticles();
	ResetBoostCursorRipple();
	ResetDestinationSuccessFlame();
	ResetDestinationSuccessMeterScale();
	ResetDestinationSuccessDiamondPulse();
	SetMagicBurstVisible(m_StartPadSuccessEffect, false);
	SetBoostPadHighlight(m_hStartPad, false);
	SetStartPadVisible(true);
	SetIntroAlpha(0.f);
	if (auto* backdrop = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hStartPadBackdrop))
	{
		backdrop->SetSize({ BOOST_PAD_BACKDROP_SIZE, BOOST_PAD_BACKDROP_SIZE });
		backdrop->SetAlpha(0.f);
		backdrop->CalcUICoord();
	}
	if (auto* icon = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hStartPad))
	{
		icon->SetSize({ BOOST_PAD_ICON_SIZE, BOOST_PAD_ICON_SIZE });
		icon->SetAlpha(0.f);
		icon->CalcUICoord();
	}
	for (BOOST_PAD& pad : m_BoostPads)
		pad.IntroRevealed = false;
	UpdateIntroPadScales(0.f);
	if (auto* introPath = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hIntroPathProgress))
	{
		introPath->SetPathProgress(0.f);
	}
	if (auto* pathProgress = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hPathProgress))
	{
		pathProgress->SetPathProgress(0.f);
		pathProgress->SetAlpha(1.f);
	}
	if (auto* spellMeter = E::CGameInstance::Get().
		GetGameObjectByHandleT<CSpellMeter>(m_hDestinationSpellMeter))
	{
		spellMeter->SetFillAmount(0.f);
	}
	UpdateArrowVisual(
		EvaluatePosition(0.f),
		m_vLastFacingDirection);
}

_bool CSpellMiniGame::CreateVisuals()
{
	const uint32_t currentLevelID =
		E::CGameInstance::Get().GetCurrentLevelID();
	const std::string currentLevel = _string("LEVEL_") +
		MagicEnumToStringView(
			static_cast<LEVEL>(currentLevelID)).data();
	const _float2 screenCenter = {
		m_vPathTopLeft.x + m_fPathSize * 0.5f,
		m_vPathTopLeft.y + m_fPathSize * 0.5f
	};

	auto createTexture = [&currentLevel](
		const std::string& objectTag,
		const std::string& resourceTag,
		const _float2& position,
		const _float2& size,
		int weight,
		const _float3& color) -> CHandle
		{
			CTextureUI::UIOBJECT_DESC desc{};
			desc.sObjectTag = objectTag;
			desc.Name = objectTag;
			desc.fX = position.x;
			desc.fY = position.y;
			desc.fSizeX = size.x;
			desc.fSizeY = size.y;
			desc.fAlpha = 1.f;
			desc.ResTag = resourceTag;
			desc.ResWeight = weight;
			desc.UIType = ETOUI(UI_TYPE::TEXUI);

			auto handle = E::CGameInstance::Get().AddGameObjectToLayer(
				currentLevel,
				"Prototype_GameObject_TextureUI",
				"Layer_UI",
				&desc);
			if (!handle)
				return {};

			auto* ui = E::CGameInstance::Get().
				GetGameObjectByHandleT<E::CUIObject>(*handle);
			if (!ui)
				return {};

			ui->SetColor(color);
			ui->SetInputLcok(true);
			return *handle;
		};
	if (!CreateBoostTrailPool(currentLevel))
		return false;

	m_hPath = createTexture(
		"SpellMiniGame_IncendioPath",
		"TEX_UI_T_SU_Incendio_Path",
		screenCenter,
		{ m_fPathSize, m_fPathSize },
		900,
		{ 0.20f, 0.20f, 0.20f });
	m_hIntroPathProgress = createTexture(
		"SpellMiniGame_IncendioPathIntro",
		"TEX_UI_T_SU_Incendio_Path",
		screenCenter,
		{ m_fPathSize, m_fPathSize },
		901,
		{ 0.62f, 0.54f, 0.28f });
	if (auto* introPath = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hIntroPathProgress))
	{
		introPath->SetPathProgressMode(true);
		introPath->SetPathProgress(0.f);
	}
	m_hPathProgress = createTexture(
		"SpellMiniGame_IncendioPathProgress",
		"TEX_UI_T_SU_Incendio_Path",
		screenCenter,
		{ m_fPathSize, m_fPathSize },
		902,
		{ 0.04f, 0.38f, 1.f });
	if (auto* pathProgress = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hPathProgress))
	{
		pathProgress->SetPathProgressMode(true);
		pathProgress->SetPathProgress(0.f);
	}
	m_hChaserPathProgress = createTexture(
		"SpellMiniGame_ChaserPathProgress",
		"TEX_UI_T_SU_Incendio_Path",
		screenCenter,
		{ m_fPathSize, m_fPathSize },
		903,
		{ 1.f, 0.02f, 0.01f });
	if (auto* chaserPath = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hChaserPathProgress))
	{
		chaserPath->SetPathProgressMode(true);
		chaserPath->SetPathProgress(0.f);
		chaserPath->SetAlpha(0.f);
	}

	CSpellMeter::UIOBJECT_DESC spellMeterDesc{};
	spellMeterDesc.sObjectTag = "SpellMiniGame_Destination_Bombarda";
	spellMeterDesc.Name = "SpellMiniGame_Destination_Bombarda";
	const _float2 destinationPosition =
		EvaluatePosition(m_fTotalPathDistance);
	if (!CreateDestinationSuccessFlame(destinationPosition, currentLevel))
		return false;
	m_hDestinationSuccessDiamond = createTexture(
		"SpellMiniGame_Destination_SuccessDiamond",
		"TEX_UI_T_FG_IndexButtonRippleGlow",
		destinationPosition,
		{
			DESTINATION_SUCCESS_DIAMOND_START_SIZE,
			DESTINATION_SUCCESS_DIAMOND_START_SIZE
		},
		905,
		{ 0.34f, 1.05f, 1.85f });
	if (auto* diamond = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hDestinationSuccessDiamond))
	{
		diamond->GetUIInfo().Rot = 45.f;
		diamond->SetAlpha(0.f);
		diamond->SetActive(false);
		diamond->SetVisible(false);
		diamond->CalcUICoord();
	}
	spellMeterDesc.fX = destinationPosition.x;
	spellMeterDesc.fY = destinationPosition.y;
	spellMeterDesc.fSizeX = DESTINATION_SPELL_METER_SIZE;
	spellMeterDesc.fSizeY = DESTINATION_SPELL_METER_SIZE;
	spellMeterDesc.fAlpha = 1.f;
	spellMeterDesc.ResTag =
		"TEX_UI_T_spellmeter_Bombarda_Overlay";
	spellMeterDesc.ResWeight = 906;
	spellMeterDesc.UIType = ETOUI(UI_TYPE::SPELLMETER);
	auto spellMeterHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_SpellMeter",
		"Layer_UI",
		&spellMeterDesc);
	if (spellMeterHandle)
	{
		m_hDestinationSpellMeter = *spellMeterHandle;
		if (auto* spellMeter = E::CGameInstance::Get().
			GetGameObjectByHandleT<CSpellMeter>(*spellMeterHandle))
		{
			spellMeter->SetSpellType(ETOUI(SPELL_TYPE::BOMBARDA));
			spellMeter->SetFillAmount(0.f);
			spellMeter->SetInputLcok(true);
		}
	}
	m_hDestinationSpellMeterBorder = createTexture(
		"SpellMiniGame_Destination_Border",
		"TEX_UI_T_FG_IndexIconBorder",
		destinationPosition,
		{
			DESTINATION_SPELL_METER_BORDER_SIZE,
			DESTINATION_SPELL_METER_BORDER_SIZE
		},
		907,
		{ 0.f, 0.f, 0.f });

	const _float2 startPosition = EvaluatePosition(0.f);
	m_hCursor = createTexture(
		"SpellMiniGame_Cursor",
		"TEX_UI_T_SU_Cursor",
		startPosition,
		{ CURSOR_SIZE, CURSOR_SIZE },
		910,
		{ 0.f, 0.f, 0.f });
	m_hBoostCursorRipple = createTexture(
		"SpellMiniGame_BoostCursorRipple",
		"TEX_UI_T_LoadingGlow",
		startPosition,
		{
			BOOST_CURSOR_RIPPLE_MIN_SIZE,
			BOOST_CURSOR_RIPPLE_MIN_SIZE
		},
		909,
		{ 0.32f, 1.05f, 1.85f });
	if (auto* ripple = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hBoostCursorRipple))
	{
		ripple->SetAlpha(0.f);
		ripple->SetActive(false);
		ripple->SetVisible(false);
		ripple->CalcUICoord();
	}
	m_hArrow = createTexture(
		"SpellMiniGame_CursorArrow",
		"TEX_UI_T_SU_CursorArrow",
		startPosition,
		{ CURSOR_ARROW_SIZE, CURSOR_ARROW_SIZE },
		910,
		{ 0.f, 0.f, 0.f });
	m_hChaserCursor = createTexture(
		"SpellMiniGame_ChaserCursor",
		"TEX_UI_T_SpellMinigame_SpeedBurstDropBack",
		startPosition,
		{ CHASER_CURSOR_SIZE, CHASER_CURSOR_SIZE },
		905,
		{ 0.f, 0.f, 0.f });
	if (auto* chaserCursor = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hChaserCursor))
	{
		chaserCursor->SetAlpha(0.f);
	}
	m_hStartPadBackdrop = createTexture(
		"SpellMiniGame_StartPad_Backdrop",
		"TEX_UI_T_SU_ButtonCalloutRing",
		startPosition,
		{ BOOST_PAD_BACKDROP_SIZE, BOOST_PAD_BACKDROP_SIZE },
		906,
		{ 0.f, 0.f, 0.f });
	m_StartPadSuccessEffect = CreateMagicBurst(startPosition);
	SetMagicBurstVisible(m_StartPadSuccessEffect, false);
	m_hStartPad = createTexture(
		"SpellMiniGame_StartPad",
		"TEX_UI_T_cbi_button_Abutton",
		startPosition,
		{ BOOST_PAD_ICON_SIZE, BOOST_PAD_ICON_SIZE },
		908,
		{ 0.f, 0.f, 0.f });

	struct BOOST_PAD_DESC
	{
		_float Progress{};
		_ubyte KeyCode{};
		const char* ResourceTag{};
		const char* ObjectTag{};
	};

	constexpr BOOST_PAD_DESC boostPadDescs[] = {
		{ 0.20f, DIK_X, "TEX_UI_T_cbi_buttonX", "SpellMiniGame_BoostPad_X_01" },
		{ 0.61f, DIK_A, "TEX_UI_T_cbi_button_Abutton", "SpellMiniGame_BoostPad_A_01" },
		{ 0.86f, DIK_A, "TEX_UI_T_cbi_button_Abutton", "SpellMiniGame_BoostPad_A_02" }
	};

	m_BoostPads.clear();
	for (const BOOST_PAD_DESC& padDesc : boostPadDescs)
	{
		const _float pathDistance =
			m_fTotalPathDistance * padDesc.Progress;
		const _float2 padPosition = EvaluatePosition(pathDistance);
		const std::string backdropTag =
			std::string{ padDesc.ObjectTag } + "_Backdrop";
		const CHandle backdropHandle = createTexture(
			backdropTag,
			"TEX_UI_T_SU_ButtonCalloutRing",
			padPosition,
			{ BOOST_PAD_BACKDROP_SIZE, BOOST_PAD_BACKDROP_SIZE },
			906,
			{ 0.f, 0.f, 0.f });
		const SUCCESS_EFFECT successEffect =
			CreateMagicBurst(padPosition);
		SetMagicBurstVisible(successEffect, false);
		const CHandle handle = createTexture(
			padDesc.ObjectTag,
			padDesc.ResourceTag,
			padPosition,
			{ BOOST_PAD_ICON_SIZE, BOOST_PAD_ICON_SIZE },
			908,
			{ 0.f, 0.f, 0.f });
		m_BoostPads.push_back({
			.PathDistance = pathDistance,
			.KeyCode = padDesc.KeyCode,
			.BackdropHandle = backdropHandle,
			.Handle = handle,
			.SuccessEffect = successEffect
			});
	}

	const _bool boostPadsCreated =
		m_BoostPads.size() == std::size(boostPadDescs) &&
		std::ranges::all_of(
			m_BoostPads,
			[](const BOOST_PAD& pad)
			{
				return E::CGameInstance::Get().
					GetGameObjectByHandleT<E::CUIObject>(pad.BackdropHandle) != nullptr &&
					E::CGameInstance::Get().
					GetGameObjectByHandleT<E::CUIObject>(pad.Handle) != nullptr &&
					E::CGameInstance::Get().
					GetGameObjectByHandleT<E::CUIObject>(pad.SuccessEffect.WispyHandle) != nullptr &&
					E::CGameInstance::Get().
					GetGameObjectByHandleT<E::CUIObject>(pad.SuccessEffect.FireHandle) != nullptr &&
					E::CGameInstance::Get().
					GetGameObjectByHandleT<E::CUIObject>(pad.SuccessEffect.CoreHandle) != nullptr &&
					E::CGameInstance::Get().
					GetGameObjectByHandleT<E::CUIObject>(pad.SuccessEffect.SmokeHandle) != nullptr;
			});

	return boostPadsCreated && E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hPath) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hIntroPathProgress) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hPathProgress) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hChaserPathProgress) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hChaserCursor) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hDestinationSuccessFlame) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hDestinationSuccessDiamond) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<CSpellMeter>(m_hDestinationSpellMeter) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hDestinationSpellMeterBorder) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hArrow) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hCursor) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hBoostCursorRipple) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hStartPadBackdrop) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_StartPadSuccessEffect.WispyHandle) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_StartPadSuccessEffect.FireHandle) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_StartPadSuccessEffect.CoreHandle) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_StartPadSuccessEffect.SmokeHandle) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hStartPad);
}

void CSpellMiniGame::DestroyVisuals()
{
	ClearTransientEffects();
	for (BOOST_PAD& pad : m_BoostPads)
	{
		DestroyMagicBurst(pad.SuccessEffect);
		DestroyUIHandle(pad.BackdropHandle);
		DestroyUIHandle(pad.Handle);
	}
	m_BoostPads.clear();
	DestroyUIHandle(m_hStartPad);
	DestroyMagicBurst(m_StartPadSuccessEffect);
	DestroyUIHandle(m_hStartPadBackdrop);
	DestroyUIHandle(m_hArrow);
	DestroyUIHandle(m_hBoostCursorRipple);
	DestroyUIHandle(m_hCursor);
	DestroyUIHandle(m_hChaserCursor);
	DestroyUIHandle(m_hDestinationSpellMeterBorder);
	DestroyUIHandle(m_hDestinationSpellMeter);
	DestroyUIHandle(m_hDestinationSuccessDiamond);
	DestroyUIHandle(m_hDestinationSuccessFlame);
	for (BOOST_TRAIL_PARTICLE& particle : m_BoostTrailParticles)
		DestroyUIHandle(particle.Handle);
	m_BoostTrailParticles.clear();
	DestroyUIHandle(m_hChaserPathProgress);
	DestroyUIHandle(m_hPathProgress);
	DestroyUIHandle(m_hIntroPathProgress);
	DestroyUIHandle(m_hPath);
}

void CSpellMiniGame::BuildIncendioPath()
{
	m_PathSamples.clear();
	m_fTotalPathDistance = 0.f;

	const auto toScreen = [this](const _float2& normalized)
		{
			return _float2{
				m_vPathTopLeft.x + normalized.x * m_fPathSize,
				m_vPathTopLeft.y + normalized.y * m_fPathSize
			};
		};

	// Incendio texture center line: lower-left -> top -> lower-right
	// -> lower-center. Coordinates are normalized against the 1024 texture.
	const _float2 start = toScreen({ 0.122f, 0.883f });
	const _float2 top = toScreen({ 0.480f, 0.086f });
	const _float2 lowerRight = toScreen({ 0.852f, 0.928f });
	const _float2 curveControl = toScreen({ 0.700f, 0.848f });
	const _float2 end = toScreen({ 0.472f, 0.853f });

	AppendLine(start, top, 120);
	AppendLine(top, lowerRight, 132);
	AppendQuadraticBezier(
		lowerRight,
		curveControl,
		end,
		64);

	if (!m_PathSamples.empty())
		m_fTotalPathDistance =
			m_PathSamples.back().AccumulatedDistance;
}

void CSpellMiniGame::AppendLine(
	const _float2& start,
	const _float2& end,
	uint32_t sampleCount)
{
	sampleCount = std::max(1u, sampleCount);
	for (uint32_t i = 0; i <= sampleCount; ++i)
	{
		if (!m_PathSamples.empty() && i == 0)
			continue;

		const _float ratio =
			static_cast<_float>(i) /
			static_cast<_float>(sampleCount);
		const _float2 position = Lerp(start, end, ratio);
		_float accumulatedDistance{};
		if (!m_PathSamples.empty())
		{
			const _float2 previous =
				m_PathSamples.back().Position;
			accumulatedDistance =
				m_PathSamples.back().AccumulatedDistance +
				Length({
					position.x - previous.x,
					position.y - previous.y
				});
		}

		m_PathSamples.push_back({
			.Position = position,
			.AccumulatedDistance = accumulatedDistance
			});
	}
}

void CSpellMiniGame::AppendQuadraticBezier(
	const _float2& start,
	const _float2& control,
	const _float2& end,
	uint32_t sampleCount)
{
	sampleCount = std::max(1u, sampleCount);
	for (uint32_t i = 0; i <= sampleCount; ++i)
	{
		if (!m_PathSamples.empty() && i == 0)
			continue;

		const _float t =
			static_cast<_float>(i) /
			static_cast<_float>(sampleCount);
		const _float oneMinusT = 1.f - t;
		const _float2 position = {
			oneMinusT * oneMinusT * start.x +
			2.f * oneMinusT * t * control.x +
			t * t * end.x,
			oneMinusT * oneMinusT * start.y +
			2.f * oneMinusT * t * control.y +
			t * t * end.y
		};
		const _float2 previous =
			m_PathSamples.back().Position;
		const _float accumulatedDistance =
			m_PathSamples.back().AccumulatedDistance +
			Length({
				position.x - previous.x,
				position.y - previous.y
			});

		m_PathSamples.push_back({
			.Position = position,
			.AccumulatedDistance = accumulatedDistance
			});
	}
}

_float2 CSpellMiniGame::EvaluatePosition(_float distance) const
{
	if (m_PathSamples.empty())
		return {};
	if (distance <= 0.f)
		return m_PathSamples.front().Position;
	if (distance >= m_fTotalPathDistance)
		return m_PathSamples.back().Position;

	const auto iter = std::lower_bound(
		m_PathSamples.begin(),
		m_PathSamples.end(),
		distance,
		[](const PATH_SAMPLE& sample, _float value)
		{
			return sample.AccumulatedDistance < value;
		});
	if (iter == m_PathSamples.begin())
		return iter->Position;

	const PATH_SAMPLE& next = *iter;
	const PATH_SAMPLE& previous = *(iter - 1);
	const _float segmentLength =
		next.AccumulatedDistance - previous.AccumulatedDistance;
	if (segmentLength <= FLT_EPSILON)
		return next.Position;

	const _float ratio =
		(distance - previous.AccumulatedDistance) / segmentLength;
	return Lerp(previous.Position, next.Position, ratio);
}

_float2 CSpellMiniGame::EvaluateForward(_float distance) const
{
	if (m_PathSamples.size() < 2)
		return { 0.f, -1.f };

	distance = std::clamp(distance, 0.f, m_fTotalPathDistance);
	const _float previousDistance = std::max(
		0.f,
		distance - CORNER_STEERING_SAMPLE_DISTANCE);
	const _float nextDistance = std::min(
		m_fTotalPathDistance,
		distance + CORNER_STEERING_SAMPLE_DISTANCE);
	const _float2 previous = EvaluatePosition(previousDistance);
	const _float2 next = EvaluatePosition(nextDistance);
	const _float2 smoothForward = Normalize({
		next.x - previous.x,
		next.y - previous.y
		});
	if (Length(smoothForward) > FLT_EPSILON)
		return smoothForward;

	return Normalize({
		m_PathSamples[1].Position.x - m_PathSamples[0].Position.x,
		m_PathSamples[1].Position.y - m_PathSamples[0].Position.y
		});
}

_bool CSpellMiniGame::CreateBoostTrailPool(
	const std::string& currentLevel)
{
	m_BoostTrailParticles.clear();
	m_BoostTrailParticles.reserve(BOOST_TRAIL_POOL_SIZE);

	for (size_t index = 0; index < BOOST_TRAIL_POOL_SIZE; ++index)
	{
		CEffectUI::FLIPBOOK_DESC desc{};
		desc.sObjectTag =
			"SpellMiniGame_BoostTrailWispy_" + std::to_string(index);
		desc.Name = desc.sObjectTag;
		desc.fX = 0.f;
		desc.fY = 0.f;
		desc.fSizeX = BOOST_TRAIL_WIDTH;
		desc.fSizeY = BOOST_TRAIL_HEIGHT;
		desc.fAlpha = 0.f;
		desc.ResTag = "TEX_VFX_T_SmokeWispy_Tile_D";
		desc.ResWeight = 908;
		desc.UIType = ETOUI(UI_TYPE::FLIPBOOK);
		desc.cellsize = 2048;
		desc.Padding = 0;
		desc.TotalFrame = 36;
		desc.Columns = 6;
		desc.Rows = 6;
		desc.Duration = BOOST_TRAIL_FLIPBOOK_DURATION;

		const auto handle = E::CGameInstance::Get().AddGameObjectToLayer(
			currentLevel,
			"Prototype_GameObject_EffectUI",
			"Layer_UI",
			&desc);
		if (!handle)
			return false;

		auto* particle = E::CGameInstance::Get().
			GetGameObjectByHandleT<CEffectUI>(*handle);
		if (!particle)
			return false;

		particle->SetColor({ 0.12f, 0.52f, 1.75f });
		particle->SetAlpha(0.f);
		particle->SetInputLcok(true);
		particle->SetActive(false);
		particle->SetVisible(false);
		particle->CalcUICoord();
		m_BoostTrailParticles.push_back({ .Handle = *handle });
	}

	return m_BoostTrailParticles.size() == BOOST_TRAIL_POOL_SIZE;
}

void CSpellMiniGame::UpdateBoostTrailEmitter(
	_float fTimeDelta,
	const _float2& position,
	const _float2& facingDirection)
{
	if (!BOOST_TRAIL_ENABLED)
	{
		m_fBoostTrailSpawnAccumulator = 0.f;
		return;
	}

	if (m_eState != STATE::RUNNING || m_fBoostTimeRemaining <= 0.f)
	{
		m_fBoostTrailSpawnAccumulator = 0.f;
		return;
	}

	m_fBoostTrailSpawnAccumulator += fTimeDelta;
	while (m_fBoostTrailSpawnAccumulator >= BOOST_TRAIL_SPAWN_INTERVAL)
	{
		m_fBoostTrailSpawnAccumulator -= BOOST_TRAIL_SPAWN_INTERVAL;
		EmitBoostTrailParticle(position, facingDirection);
	}
}

void CSpellMiniGame::UpdateBoostTrailParticles(_float fTimeDelta)
{
	for (BOOST_TRAIL_PARTICLE& particle : m_BoostTrailParticles)
	{
		if (particle.RemainingTime <= 0.f)
			continue;

		auto* effect = E::CGameInstance::Get().
			GetGameObjectByHandleT<CEffectUI>(particle.Handle);
		if (!effect || effect->GetPendingDestroy())
		{
			particle.RemainingTime = 0.f;
			continue;
		}

		particle.RemainingTime = std::max(
			0.f,
			particle.RemainingTime - fTimeDelta);
		const _float lifeRatio = std::clamp(
			particle.Duration > FLT_EPSILON ?
			particle.RemainingTime / particle.Duration : 0.f,
			0.f,
			1.f);
		const _float fadeRatio =
			lifeRatio * (2.f - lifeRatio);
		const _float elapsedRatio = 1.f - lifeRatio;
		const _float driftRatio = elapsedRatio * elapsedRatio;
		const _float2 driftedPosition = Lerp(
			particle.SpawnPosition,
			particle.CenterPosition,
			driftRatio);
		const _float waveLinearRatio = std::clamp(
			(elapsedRatio - 0.25f) / 0.75f,
			0.f,
			1.f);
		const _float waveRatio = waveLinearRatio * waveLinearRatio *
			(3.f - 2.f * waveLinearRatio);
		const _float waveOffset = sinf(
			particle.WavePhase +
			elapsedRatio * BOOST_TRAIL_WAVE_TIME_SPEED) *
			BOOST_TRAIL_WAVE_AMPLITUDE * waveRatio;
		const _float2 currentPosition = {
			driftedPosition.x +
				particle.LateralDirection.x * waveOffset,
			driftedPosition.y +
				particle.LateralDirection.y * waveOffset
		};
		const _float sizeScale = std::lerp(
			1.f,
			BOOST_TRAIL_END_SIZE_SCALE,
			elapsedRatio);
		effect->SetPos(currentPosition);
		effect->SetAlpha(BOOST_TRAIL_MAX_ALPHA * fadeRatio);
		effect->SetSize({
			BOOST_TRAIL_WIDTH * sizeScale,
			BOOST_TRAIL_HEIGHT * sizeScale
			});
		effect->CalcUICoord();
		if (particle.RemainingTime <= 0.f)
		{
			effect->SetAlpha(0.f);
			effect->SetActive(false);
			effect->SetVisible(false);
		}
	}
}

void CSpellMiniGame::EmitBoostTrailParticle(
	const _float2& position,
	const _float2& facingDirection)
{
	if (m_BoostTrailParticles.empty())
		return;

	BOOST_TRAIL_PARTICLE& particle =
		m_BoostTrailParticles[m_iNextBoostTrailParticle];
	m_iNextBoostTrailParticle =
		(m_iNextBoostTrailParticle + 1) % m_BoostTrailParticles.size();
	auto* effect = E::CGameInstance::Get().
		GetGameObjectByHandleT<CEffectUI>(particle.Handle);
	if (!effect)
		return;

	const _float2 direction = Normalize(facingDirection);
	const _float2 lateralDirection = { -direction.y, direction.x };
	const _float2 anchoredPosition = {
		position.x - direction.x *
			BOOST_TRAIL_SOURCE_ANCHOR_OFFSET,
		position.y - direction.y *
			BOOST_TRAIL_SOURCE_ANCHOR_OFFSET
	};
	static std::mt19937 generator{ std::random_device{}() };
	static std::uniform_real_distribution<_float> lateralOffsetDistribution{
		BOOST_TRAIL_LATERAL_OFFSET_MIN,
		BOOST_TRAIL_LATERAL_OFFSET_MAX
	};
	const _float lateralOffset = lateralOffsetDistribution(generator);
	const _float side = m_bNextBoostTrailLeft ? -1.f : 1.f;
	m_bNextBoostTrailLeft = !m_bNextBoostTrailLeft;
	const _float2 driftTarget = {
		anchoredPosition.x + lateralDirection.x * lateralOffset * side,
		anchoredPosition.y + lateralDirection.y * lateralOffset * side
	};
	const _float angle = XMConvertToDegrees(
		atan2f(direction.y, direction.x));

	effect->Restart();
	// Place the first frame's visible smoke center on the cursor. The atlas
	// frame itself is not visually centered, so the quad needs a local offset.
	effect->SetPos(anchoredPosition);
	// SmokeWispy animates from the bottom toward the top of each frame.
	// Aim that visual motion opposite to the cursor's travel direction.
	effect->GetUIInfo().Rot = 90.f - angle;
	effect->SetSize({ BOOST_TRAIL_WIDTH, BOOST_TRAIL_HEIGHT });
	effect->SetColor({ 0.12f, 0.52f, 1.75f });
	effect->SetAlpha(BOOST_TRAIL_MAX_ALPHA);
	effect->SetActive(true);
	effect->SetVisible(true);
	effect->CalcUICoord();
	particle.RemainingTime = BOOST_TRAIL_PARTICLE_DURATION;
	particle.Duration = BOOST_TRAIL_PARTICLE_DURATION;
	particle.SpawnPosition = anchoredPosition;
	particle.CenterPosition = driftTarget;
	particle.LateralDirection = lateralDirection;
	particle.WavePhase = m_fBoostTrailWavePhase;
	m_fBoostTrailWavePhase = fmodf(
		m_fBoostTrailWavePhase + BOOST_TRAIL_WAVE_PHASE_STEP,
		XM_2PI);
}

void CSpellMiniGame::ClearBoostTrailParticles()
{
	m_fBoostTrailSpawnAccumulator = 0.f;
	m_fBoostTrailWavePhase = 0.f;
	m_iNextBoostTrailParticle = 0;
	m_bNextBoostTrailLeft = true;
	for (BOOST_TRAIL_PARTICLE& particle : m_BoostTrailParticles)
	{
		particle.RemainingTime = 0.f;
		particle.Duration = 0.f;
		particle.SpawnPosition = {};
		particle.CenterPosition = {};
		particle.LateralDirection = {};
		particle.WavePhase = 0.f;
		if (auto* effect = E::CGameInstance::Get().
			GetGameObjectByHandleT<CEffectUI>(particle.Handle))
		{
			effect->SetAlpha(0.f);
			effect->SetActive(false);
			effect->SetVisible(false);
		}
	}
}

void CSpellMiniGame::UpdateArrowVisual(
	const _float2& position,
	const _float2& facingDirection)
{
	const _float angle = XMConvertToDegrees(
		atan2f(facingDirection.y, facingDirection.x));
	// Screen Y grows downward while the UI transform uses upward Y.
	const _float uiRotation = 90.f - angle;

	if (auto* cursor = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hCursor))
	{
		cursor->SetPos(position);
		// The cursor texture's forward axis is opposite to CursorArrow.
		cursor->GetUIInfo().Rot = uiRotation + 180.f;
		cursor->CalcUICoord();
	}

	if (auto* arrow = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hArrow))
	{
		const _float2 orbitPosition = {
			position.x + facingDirection.x * CURSOR_ARROW_ORBIT_RADIUS,
			position.y + facingDirection.y * CURSOR_ARROW_ORBIT_RADIUS
		};
		arrow->SetPos(orbitPosition);
		arrow->GetUIInfo().Rot = uiRotation;
		arrow->CalcUICoord();
	}
}

void CSpellMiniGame::TryActivateBoostPad(
	_bool aPressed,
	_bool xPressed)
{
	for (BOOST_PAD& pad : m_BoostPads)
	{
		if (pad.Consumed ||
			fabsf(m_fPathDistance - pad.PathDistance) >
			BOOST_PAD_TRIGGER_RANGE)
		{
			continue;
		}

		const _bool correctKeyPressed =
			(pad.KeyCode == DIK_A && aPressed) ||
			(pad.KeyCode == DIK_X && xPressed);
		if (!correctKeyPressed)
			continue;

		pad.Consumed = true;
		ActivateBoost();
		PlayMagicBurst(pad.SuccessEffect, pad.Handle);
		break;
	}
}

void CSpellMiniGame::ActivateBoost()
{
	m_fBoostTimeRemaining = BOOST_DURATION;
	// Prime the accumulator so the first puff appears immediately.
	m_fBoostTrailSpawnAccumulator = BOOST_TRAIL_SPAWN_INTERVAL;
	PlayBoostCursorRipple();
}

void CSpellMiniGame::PlayBoostCursorRipple()
{
	auto* ripple = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hBoostCursorRipple);
	if (!ripple || ripple->GetPendingDestroy())
		return;

	m_fBoostCursorRippleElapsed = 0.f;
	m_bBoostCursorRippleActive = true;
	ripple->SetPos(EvaluatePosition(m_fPathDistance));
	ripple->SetSize({
		BOOST_CURSOR_RIPPLE_MIN_SIZE,
		BOOST_CURSOR_RIPPLE_MIN_SIZE
		});
	ripple->SetColor({ 0.32f, 1.05f, 1.85f });
	ripple->SetAlpha(1.f);
	ripple->SetActive(true);
	ripple->SetVisible(true);
	ripple->CalcUICoord();
}

void CSpellMiniGame::UpdateBoostCursorRipple(_float fTimeDelta)
{
	if (!m_bBoostCursorRippleActive)
		return;

	auto* ripple = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hBoostCursorRipple);
	if (!ripple || ripple->GetPendingDestroy())
	{
		m_bBoostCursorRippleActive = false;
		return;
	}

	m_fBoostCursorRippleElapsed += std::max(0.f, fTimeDelta);
	if (m_fBoostCursorRippleElapsed >=
		BOOST_CURSOR_RIPPLE_TOTAL_DURATION)
	{
		ResetBoostCursorRipple();
		return;
	}

	const _float cycleDuration =
		BOOST_CURSOR_RIPPLE_HALF_DURATION * 2.f;
	const _float cycleTime = fmodf(
		m_fBoostCursorRippleElapsed,
		cycleDuration);
	const _bool expanding =
		cycleTime < BOOST_CURSOR_RIPPLE_HALF_DURATION;
	const _float halfRatio = std::clamp(
		expanding ?
			cycleTime / BOOST_CURSOR_RIPPLE_HALF_DURATION :
			(cycleTime - BOOST_CURSOR_RIPPLE_HALF_DURATION) /
				BOOST_CURSOR_RIPPLE_HALF_DURATION,
		0.f,
		1.f);
	const _float easedRatio = halfRatio * halfRatio *
		(3.f - 2.f * halfRatio);
	const _float size = expanding ?
		std::lerp(
			BOOST_CURSOR_RIPPLE_MIN_SIZE,
			BOOST_CURSOR_RIPPLE_MAX_SIZE,
			easedRatio) :
		std::lerp(
			BOOST_CURSOR_RIPPLE_MAX_SIZE,
			BOOST_CURSOR_RIPPLE_MIN_SIZE,
			easedRatio);
	const _float alpha = expanding ?
		1.f - easedRatio : easedRatio;

	ripple->SetPos(EvaluatePosition(m_fPathDistance));
	ripple->SetSize({ size, size });
	ripple->SetAlpha(alpha);
	ripple->CalcUICoord();
}

void CSpellMiniGame::ResetBoostCursorRipple()
{
	m_fBoostCursorRippleElapsed = 0.f;
	m_bBoostCursorRippleActive = false;
	if (auto* ripple = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hBoostCursorRipple))
	{
		ripple->SetSize({
			BOOST_CURSOR_RIPPLE_MIN_SIZE,
			BOOST_CURSOR_RIPPLE_MIN_SIZE
			});
		ripple->SetAlpha(0.f);
		ripple->SetActive(false);
		ripple->SetVisible(false);
		ripple->CalcUICoord();
	}
}

void CSpellMiniGame::UpdateChaser(
	_float movementDelta,
	_float timerDelta)
{
	if (m_eState != STATE::RUNNING)
		return;

	if (!m_bChaserActive)
	{
		m_fChaserStartDelayRemaining = std::max(
			0.f,
			m_fChaserStartDelayRemaining - timerDelta);
		if (m_fChaserStartDelayRemaining > 0.f)
			return;

		m_bChaserActive = true;
		m_fChaserPathDistance = 0.f;
		SetChaserVisible(true);
	}

	m_fChaserPathDistance = std::min(
		m_fTotalPathDistance,
		m_fChaserPathDistance + CHASER_MOVE_SPEED * movementDelta);

	if (auto* chaserPath = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hChaserPathProgress))
	{
		const _float progress = m_fTotalPathDistance > FLT_EPSILON ?
			m_fChaserPathDistance / m_fTotalPathDistance : 0.f;
		chaserPath->SetPathProgress(progress);
	}

	if (auto* chaserCursor = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hChaserCursor))
	{
		chaserCursor->SetPos(EvaluatePosition(m_fChaserPathDistance));
		chaserCursor->CalcUICoord();
	}

	if (m_fChaserPathDistance + CHASER_COLLISION_DISTANCE >=
		m_fPathDistance)
	{
		ResetToStart();
	}
}

void CSpellMiniGame::ResetChaser()
{
	m_bChaserActive = false;
	m_fChaserPathDistance = 0.f;
	m_fChaserStartDelayRemaining = CHASER_START_DELAY;
	if (auto* chaserPath = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hChaserPathProgress))
	{
		chaserPath->SetPathProgress(0.f);
	}
	SetChaserVisible(false);
}

void CSpellMiniGame::SetChaserVisible(_bool visible)
{
	const _float alpha = visible ? 1.f : 0.f;
	if (auto* chaserPath = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hChaserPathProgress))
	{
		chaserPath->SetAlpha(alpha);
	}

	if (auto* chaserCursor = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hChaserCursor))
	{
		chaserCursor->SetAlpha(alpha);
	}
}

void CSpellMiniGame::ResetBoostPads()
{
	for (BOOST_PAD& pad : m_BoostPads)
	{
		pad.Consumed = false;
		SetMagicBurstVisible(pad.SuccessEffect, false);
		SetBoostPadHighlight(pad.Handle, false);
		SetUIHierarchyVisible(pad.BackdropHandle, true);
		if (auto* padUI = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(pad.Handle))
		{
			padUI->SetActive(true);
			padUI->SetVisible(true);
		}
	}
}

void CSpellMiniGame::SetStartPadVisible(_bool visible)
{
	SetUIHierarchyVisible(m_hStartPadBackdrop, visible);

	if (auto* startPad = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hStartPad))
	{
		startPad->SetActive(visible);
		startPad->SetVisible(visible);
	}
}

void CSpellMiniGame::SetCursorVisible(_bool visible)
{
	const _float alpha = visible ? 1.f : 0.f;
	if (auto* cursor = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hCursor))
	{
		cursor->SetAlpha(alpha);
	}

	if (auto* arrow = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hArrow))
	{
		arrow->SetAlpha(alpha);
	}
}

_bool CSpellMiniGame::CreateDestinationSuccessFlame(
	const _float2& destinationPosition,
	const std::string& currentLevel)
{
	CEffectUI::FLIPBOOK_DESC desc{};
	desc.sObjectTag = "SpellMiniGame_Destination_SuccessFlame";
	desc.Name = desc.sObjectTag;
	desc.fX = destinationPosition.x;
	desc.fY = destinationPosition.y + DESTINATION_SUCCESS_FLAME_OFFSET_Y;
	desc.fSizeX = DESTINATION_SUCCESS_FLAME_WIDTH;
	desc.fSizeY = DESTINATION_SUCCESS_FLAME_HEIGHT;
	desc.fAlpha = 0.f;
	desc.ResTag = "TEX_UI_T_PointFlame";
	desc.ResWeight = 904;
	desc.UIType = ETOUI(UI_TYPE::FLIPBOOK);
	desc.cellsize = 128;
	desc.Padding = 0;
	desc.TotalFrame = 1;
	desc.Columns = 1;
	desc.Rows = 1;
	desc.Duration = 1.f;

	const auto handle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_EffectUI",
		"Layer_UI",
		&desc);
	if (!handle)
		return false;

	m_hDestinationSuccessFlame = *handle;
	auto* flame = E::CGameInstance::Get().
		GetGameObjectByHandleT<CEffectUI>(m_hDestinationSuccessFlame);
	if (!flame)
		return false;

	flame->SetColor({ 1.f, 1.f, 1.f });
	flame->SetAlpha(0.f);
	flame->SetInputLcok(true);
	flame->SetActive(false);
	flame->SetVisible(false);
	flame->CalcUICoord();
	return true;
}

void CSpellMiniGame::PlayDestinationSuccessFlame()
{
	auto* flame = E::CGameInstance::Get().
		GetGameObjectByHandleT<CEffectUI>(m_hDestinationSuccessFlame);
	if (!flame)
		return;

	flame->SetActive(true);
	flame->SetVisible(true);
	flame->SetAlpha(0.f);
	flame->SetSize({
		DESTINATION_SUCCESS_FLAME_WIDTH,
		DESTINATION_SUCCESS_FLAME_HEIGHT
		});
	flame->CalcUICoord();

	if (auto* tween = flame->GetTweenCom())
	{
		tween->ClearTweens();
		tween->PlayTween(
			0.f,
			1.f,
			DESTINATION_SUCCESS_FLAME_FADE_TIME,
			[flame](_float alpha)
			{
				flame->SetAlpha(alpha);
			},
			nullptr,
			EEaseType::EaseOutQuad);

		tween->PlayTween(
			1.f,
			DESTINATION_SUCCESS_FLAME_SCALE,
			DESTINATION_SUCCESS_ANIMATION_DURATION,
			[flame](_float scale)
			{
				flame->SetSize({
					DESTINATION_SUCCESS_FLAME_WIDTH * scale,
					DESTINATION_SUCCESS_FLAME_HEIGHT * scale
					});
				flame->CalcUICoord();
			},
			nullptr,
			EEaseType::Linear);
	}
	else
	{
		flame->SetAlpha(1.f);
		flame->SetSize({
			DESTINATION_SUCCESS_FLAME_WIDTH * DESTINATION_SUCCESS_FLAME_SCALE,
			DESTINATION_SUCCESS_FLAME_HEIGHT * DESTINATION_SUCCESS_FLAME_SCALE
			});
		flame->CalcUICoord();
	}
}

void CSpellMiniGame::ResetDestinationSuccessFlame()
{
	auto* flame = E::CGameInstance::Get().
		GetGameObjectByHandleT<CEffectUI>(m_hDestinationSuccessFlame);
	if (!flame)
		return;

	if (auto* tween = flame->GetTweenCom())
		tween->ClearTweens();
	flame->SetAlpha(0.f);
	flame->SetSize({
		DESTINATION_SUCCESS_FLAME_WIDTH,
		DESTINATION_SUCCESS_FLAME_HEIGHT
		});
	const _float2 destinationPosition =
		EvaluatePosition(m_fTotalPathDistance);
	flame->GetUIInfo().fX = destinationPosition.x;
	flame->GetUIInfo().fY =
		destinationPosition.y + DESTINATION_SUCCESS_FLAME_OFFSET_Y;
	flame->SetActive(false);
	flame->SetVisible(false);
	flame->CalcUICoord();
}

void CSpellMiniGame::PlayDestinationSuccessMeterScale()
{
	auto playScale = [](
		E::CUIObject* ui,
		_float startSize,
		_float endSize)
		{
			if (!ui)
				return;

			ui->SetSize({ startSize, startSize });
			ui->CalcUICoord();
			if (auto* tween = ui->GetTweenCom())
			{
				tween->ClearTweens();
				tween->PlayTween(
					startSize,
					endSize,
					DESTINATION_SUCCESS_ANIMATION_DURATION,
					[ui](_float size)
					{
						ui->SetSize({ size, size });
						ui->CalcUICoord();
					},
					nullptr,
					EEaseType::Linear);
			}
			else
			{
				ui->SetSize({ endSize, endSize });
				ui->CalcUICoord();
			}
		};

	playScale(
		E::CGameInstance::Get().GetGameObjectByHandleT<CSpellMeter>(
			m_hDestinationSpellMeter),
		DESTINATION_SPELL_METER_SIZE,
		DESTINATION_SPELL_METER_SIZE * DESTINATION_SUCCESS_METER_SCALE);
	playScale(
		E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(
			m_hDestinationSpellMeterBorder),
		DESTINATION_SPELL_METER_BORDER_SIZE,
		DESTINATION_SPELL_METER_BORDER_SIZE * DESTINATION_SUCCESS_METER_SCALE);
}

void CSpellMiniGame::ResetDestinationSuccessMeterScale()
{
	const _float2 destinationPosition =
		EvaluatePosition(m_fTotalPathDistance);
	auto resetScale = [destinationPosition](E::CUIObject* ui, _float size)
		{
			if (!ui)
				return;

			if (auto* tween = ui->GetTweenCom())
				tween->ClearTweens();
			ui->GetUIInfo().fX = destinationPosition.x;
			ui->GetUIInfo().fY = destinationPosition.y;
			ui->SetSize({ size, size });
			ui->CalcUICoord();
		};

	resetScale(
		E::CGameInstance::Get().GetGameObjectByHandleT<CSpellMeter>(
			m_hDestinationSpellMeter),
		DESTINATION_SPELL_METER_SIZE);
	resetScale(
		E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(
			m_hDestinationSpellMeterBorder),
		DESTINATION_SPELL_METER_BORDER_SIZE);
}

void CSpellMiniGame::PlayDestinationSuccessDiamondPulse()
{
	auto* diamond = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hDestinationSuccessDiamond);
	if (!diamond)
		return;

	diamond->SetActive(true);
	diamond->SetVisible(true);
	diamond->SetAlpha(1.f);
	diamond->SetSize({
		DESTINATION_SUCCESS_DIAMOND_START_SIZE,
		DESTINATION_SUCCESS_DIAMOND_START_SIZE
		});
	diamond->CalcUICoord();

	if (auto* tween = diamond->GetTweenCom())
	{
		tween->ClearTweens();
		tween->PlayTween(
			DESTINATION_SUCCESS_DIAMOND_START_SIZE,
			DESTINATION_SUCCESS_DIAMOND_END_SIZE,
			DESTINATION_SUCCESS_DIAMOND_DURATION,
			[diamond](_float size)
			{
				diamond->SetSize({ size, size });
				diamond->CalcUICoord();
			},
			nullptr,
			EEaseType::Linear);

		tween->PlayTween(
			1.f,
			0.f,
			DESTINATION_SUCCESS_DIAMOND_DURATION,
			[diamond](_float alpha)
			{
				diamond->SetAlpha(alpha);
			},
			[diamond]()
			{
				diamond->SetAlpha(0.f);
				diamond->SetActive(false);
				diamond->SetVisible(false);
			},
			EEaseType::Linear);
	}
	else
	{
		diamond->SetAlpha(0.f);
		diamond->SetActive(false);
		diamond->SetVisible(false);
	}
}

void CSpellMiniGame::ResetDestinationSuccessDiamondPulse()
{
	auto* diamond = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hDestinationSuccessDiamond);
	if (!diamond)
		return;

	if (auto* tween = diamond->GetTweenCom())
		tween->ClearTweens();
	diamond->SetAlpha(0.f);
	diamond->SetSize({
		DESTINATION_SUCCESS_DIAMOND_START_SIZE,
		DESTINATION_SUCCESS_DIAMOND_START_SIZE
		});
	diamond->SetActive(false);
	diamond->SetVisible(false);
	diamond->CalcUICoord();
}

void CSpellMiniGame::UpdateCompletionSequence(_float fTimeDelta)
{
	m_fCompletionPhaseElapsed += fTimeDelta;

	switch (m_eCompletionPhase)
	{
	case COMPLETION_PHASE::SUCCESS_ANIMATION:
		if (m_fCompletionPhaseElapsed >=
			DESTINATION_SUCCESS_ANIMATION_DURATION)
		{
			m_eCompletionPhase = COMPLETION_PHASE::CENTER_DELAY;
			m_fCompletionPhaseElapsed = 0.f;
		}
		break;

	case COMPLETION_PHASE::CENTER_DELAY:
		if (m_fCompletionPhaseElapsed >=
			DESTINATION_SUCCESS_CENTER_DELAY)
		{
			PlayCompletionCenterTransition();
			m_eCompletionPhase = COMPLETION_PHASE::CENTER_MOVE;
			m_fCompletionPhaseElapsed = 0.f;
		}
		break;

	case COMPLETION_PHASE::CENTER_MOVE:
		if (m_fCompletionPhaseElapsed >=
			DESTINATION_SUCCESS_CENTER_MOVE_DURATION)
		{
			m_eCompletionPhase = COMPLETION_PHASE::CENTER_HOLD;
			m_fCompletionPhaseElapsed = 0.f;
		}
		break;

	case COMPLETION_PHASE::CENTER_HOLD:
		if (m_fCompletionPhaseElapsed >=
			DESTINATION_SUCCESS_CENTER_HOLD_DURATION)
		{
			PlayCompletionExitTransition();
			m_eCompletionPhase = COMPLETION_PHASE::EXITING;
			m_fCompletionPhaseElapsed = 0.f;
		}
		break;

	case COMPLETION_PHASE::EXITING:
		if (m_fCompletionPhaseElapsed >=
			DESTINATION_SUCCESS_EXIT_DURATION)
		{
			ShowSuccessAlarm();
			SetPendingDestroy();
			m_eCompletionPhase = COMPLETION_PHASE::NONE;
			m_fCompletionPhaseElapsed = 0.f;
		}
		break;

	case COMPLETION_PHASE::NONE:
	default:
		break;
	}
}

void CSpellMiniGame::ShowSuccessAlarm()
{
	const std::vector<CHandle> alarmRoots =
		GET_SINGLE(UIManager)->LoadPrefab("SpellAlam");
	uint32_t flameIndex = 0;
	_float minX = FLT_MAX;
	_float minY = FLT_MAX;
	_float maxX = -FLT_MAX;
	_float maxY = -FLT_MAX;
	_bool hasBounds = false;

	const auto configureAlarm = [
		&flameIndex,
		&minX,
		&minY,
		&maxX,
		&maxY,
		&hasBounds](
		auto&& self,
		CHandle rootHandle,
		const _float2& parentPosition,
		_float parentRotation,
		_float parentScale,
		_bool hasParent) -> void
		{
			auto* root = E::CGameInstance::Get().
				GetGameObjectByHandleT<E::CUIObject>(rootHandle);
			if (!root)
				return;

			const UI_INFO& info = root->GetUIInfo();
			_float2 position = root->GetPos();
			_float rotation = info.Rot;
			_float inheritedScale = root->GetScaleRatio();
			if (hasParent)
			{
				const _float radians = XMConvertToRadians(parentRotation);
				const _float cosine = cosf(radians);
				const _float sine = sinf(radians);
				const _float localX = info.LocalX * parentScale;
				const _float localY = info.LocalY * parentScale;
				position = {
					parentPosition.x + localX * cosine + localY * sine,
					parentPosition.y - localX * sine + localY * cosine
				};
				rotation = parentRotation + info.LocalRot;
				inheritedScale = parentScale;
			}

			const _float objectScale =
				inheritedScale * root->GetLocalScaleRatio();
			const _float width = fabsf(info.SizeX * objectScale);
			const _float height = fabsf(info.SizeY * objectScale);
			const _float radians = XMConvertToRadians(rotation);
			const _float halfWidth =
				(fabsf(cosf(radians)) * width +
					fabsf(sinf(radians)) * height) * 0.5f;
			const _float halfHeight =
				(fabsf(sinf(radians)) * width +
					fabsf(cosf(radians)) * height) * 0.5f;
			minX = std::min(minX, position.x - halfWidth);
			minY = std::min(minY, position.y - halfHeight);
			maxX = std::max(maxX, position.x + halfWidth);
			maxY = std::max(maxY, position.y + halfHeight);
			hasBounds = true;

			if (std::string_view(root->GetName()) == "Flame")
			{
				if (auto* flame = dynamic_cast<CTextureUI*>(root))
					flame->SetSpellAlarmFlame(flameIndex++);
			}

			for (const CHandle childHandle : root->GetChildren())
			{
				self(
					self,
					childHandle,
					position,
					rotation,
					inheritedScale,
					true);
			}
		};

	for (const CHandle handle : alarmRoots)
	{
		auto* alarmRoot = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(handle);
		if (!alarmRoot)
		{
			continue;
		}
		configureAlarm(
			configureAlarm,
			handle,
			{},
			0.f,
			1.f,
			false);
		alarmRoot->SetAlpha(0.f);

		GET_SINGLE(UIManager)->PlayFadeIn(
			handle,
			SUCCESS_ALARM_DELAY,
			SUCCESS_ALARM_FADE_IN_DURATION);
		GET_SINGLE(UIManager)->PlayFadeOutDelete(
			handle,
			SUCCESS_ALARM_DELAY +
			SUCCESS_ALARM_FADE_IN_DURATION +
			SUCCESS_ALARM_HOLD_DURATION,
			SUCCESS_ALARM_FADE_OUT_DURATION);
	}

	if (!hasBounds)
		return;

	const _float2 alarmCenter = {
		(minX + maxX) * 0.5f,
		(minY + maxY) * 0.5f +
			SUCCESS_ALARM_EFFECT_CENTER_OFFSET_Y
	};
	const _float2 alarmSize = {
		std::max(1.f, maxX - minX),
		std::max(1.f, maxY - minY) *
			SUCCESS_ALARM_EFFECT_HEIGHT_SCALE
	};
	const std::vector<CHandle> smokeRoots =
		GET_SINGLE(UIManager)->LoadPrefab("SmokeBurst");
	for (const CHandle smokeHandle : smokeRoots)
	{
		auto* smoke = E::CGameInstance::Get().
			GetGameObjectByHandleT<CEffectUI>(smokeHandle);
		if (!smoke)
			continue;

		smoke->Restart();
		smoke->SetPos(alarmCenter);
		smoke->SetSize(alarmSize);
		smoke->GetUIInfo().Weight = SUCCESS_ALARM_SMOKE_WEIGHT;
		smoke->SetColor({ 1.12f, 0.96f, 0.60f });
		smoke->SetAlpha(0.f);
		smoke->SetInputLcok(true);
		smoke->SetActive(true);
		smoke->SetVisible(true);
		smoke->CalcUICoord();

		if (auto* tween = smoke->GetTweenCom())
		{
			tween->ClearTweens();
			tween->PlayTween(
				0.f,
				SUCCESS_ALARM_SMOKE_ALPHA,
				SUCCESS_ALARM_SMOKE_FADE_IN_DURATION,
				[smokeHandle](_float alpha)
				{
					if (auto* currentSmoke = E::CGameInstance::Get().
						GetGameObjectByHandleT<E::CUIObject>(smokeHandle))
					{
						currentSmoke->SetAlpha(alpha);
					}
				},
				nullptr,
				EEaseType::EaseOutQuad);
			tween->PlayTween(
				SUCCESS_ALARM_SMOKE_ALPHA,
				0.f,
				SUCCESS_ALARM_SMOKE_FADE_DURATION,
				[smokeHandle](_float alpha)
				{
					if (auto* currentSmoke = E::CGameInstance::Get().
						GetGameObjectByHandleT<E::CUIObject>(smokeHandle))
					{
						currentSmoke->SetAlpha(alpha);
					}
				},
				[smokeHandle]()
				{
					if (GetSafeUI(smokeHandle))
						GET_SINGLE(UIManager)->DeleteUIRecursive(smokeHandle);
				},
				EEaseType::Linear,
				SUCCESS_ALARM_SMOKE_DURATION -
					SUCCESS_ALARM_SMOKE_FADE_DURATION);
		}
		else
		{
			smoke->SetAlpha(SUCCESS_ALARM_SMOKE_ALPHA);
		}
	}

	const std::string currentLevel = _string("LEVEL_") +
		MagicEnumToStringView(static_cast<LEVEL>(
			E::CGameInstance::Get().GetCurrentLevelID())).data();
	const _float splatterBaseSize = std::min(
		alarmSize.x,
		alarmSize.y);
	CTextureUI::UIOBJECT_DESC splatterDesc{};
	splatterDesc.sObjectTag = "SpellAlarm_MagicSplatter";
	splatterDesc.Name = splatterDesc.sObjectTag;
	splatterDesc.fX = alarmCenter.x;
	splatterDesc.fY = alarmCenter.y;
	splatterDesc.fSizeX =
		splatterBaseSize * SUCCESS_ALARM_SPLATTER_START_SCALE;
	splatterDesc.fSizeY =
		splatterBaseSize * SUCCESS_ALARM_SPLATTER_START_SCALE;
	splatterDesc.fAlpha = SUCCESS_ALARM_SPLATTER_ALPHA;
	splatterDesc.ResTag = "TEX_UI_T_MagicSplatter";
	splatterDesc.ResWeight = SUCCESS_ALARM_SPLATTER_WEIGHT;
	splatterDesc.UIType = ETOUI(UI_TYPE::TEXUI);
	const auto splatterHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_TextureUI",
		"Layer_UI",
		&splatterDesc);
	if (splatterHandle)
	{
		auto* splatter = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(*splatterHandle);
		if (splatter)
		{
			splatter->SetColor({ 1.18f, 1.00f, 0.58f });
			splatter->SetAdditiveBlend(true);
			splatter->SetInputLcok(true);
			splatter->CalcUICoord();

			if (auto* tween = splatter->GetTweenCom())
			{
				tween->ClearTweens();
				tween->PlayTween(
					0.f,
					SUCCESS_ALARM_SPLATTER_ROTATION,
					SUCCESS_ALARM_SPLATTER_DURATION,
					[handle = *splatterHandle](_float rotation)
					{
						if (auto* currentSplatter = E::CGameInstance::Get().
							GetGameObjectByHandleT<E::CUIObject>(handle))
						{
							currentSplatter->GetUIInfo().Rot = rotation;
							currentSplatter->CalcUICoord();
						}
					},
					nullptr,
					EEaseType::EaseOutQuad);
				tween->PlayTween(
					SUCCESS_ALARM_SPLATTER_START_SCALE,
					SUCCESS_ALARM_SPLATTER_END_SCALE,
					SUCCESS_ALARM_SPLATTER_DURATION,
					[handle = *splatterHandle, splatterBaseSize](_float scale)
					{
						if (auto* currentSplatter = E::CGameInstance::Get().
							GetGameObjectByHandleT<E::CUIObject>(handle))
						{
							currentSplatter->SetSize({
								splatterBaseSize * scale,
								splatterBaseSize * scale
								});
							currentSplatter->CalcUICoord();
						}
					},
					nullptr,
					EEaseType::EaseOutQuad);
				tween->PlayTween(
					SUCCESS_ALARM_SPLATTER_ALPHA,
					0.f,
					SUCCESS_ALARM_SPLATTER_DURATION,
					[handle = *splatterHandle](_float alpha)
					{
						if (auto* currentSplatter = E::CGameInstance::Get().
							GetGameObjectByHandleT<E::CUIObject>(handle))
						{
							currentSplatter->SetAlpha(alpha);
						}
					},
					[handle = *splatterHandle]()
					{
						if (GetSafeUI(handle))
							GET_SINGLE(UIManager)->DeleteUIRecursive(handle);
					},
					EEaseType::EaseOutQuad);
			}
		}
	}
}

void CSpellMiniGame::PlayCompletionCenterTransition()
{
	const _float2 clientSize =
		E::CGameInstance::Get().GetClientScreenSize();
	const _float2 screenCenter = {
		clientSize.x * 0.5f,
		clientSize.y * 0.5f
	};

	auto moveUI = [](
		CHandle handle,
		const _float2& targetPosition)
		{
			auto* ui = E::CGameInstance::Get().
				GetGameObjectByHandleT<E::CUIObject>(handle);
			if (!ui)
				return;

			auto* tween = ui->GetTweenCom();
			if (!tween)
			{
				ui->GetUIInfo().fX = targetPosition.x;
				ui->GetUIInfo().fY = targetPosition.y;
				ui->CalcUICoord();
				return;
			}

			tween->ClearTweens();
			const _float startX = ui->GetUIInfo().fX;
			const _float startY = ui->GetUIInfo().fY;
			tween->PlayTween(
				startX,
				targetPosition.x,
				DESTINATION_SUCCESS_CENTER_MOVE_DURATION,
				[handle](_float currentX)
				{
					if (auto* currentUI = E::CGameInstance::Get().
						GetGameObjectByHandleT<E::CUIObject>(handle))
					{
						currentUI->GetUIInfo().fX = currentX;
						currentUI->CalcUICoord();
					}
				},
				nullptr,
				EEaseType::EaseOutQuad);
			tween->PlayTween(
				startY,
				targetPosition.y,
				DESTINATION_SUCCESS_CENTER_MOVE_DURATION,
				[handle](_float currentY)
				{
					if (auto* currentUI = E::CGameInstance::Get().
						GetGameObjectByHandleT<E::CUIObject>(handle))
					{
						currentUI->GetUIInfo().fY = currentY;
						currentUI->CalcUICoord();
					}
				},
				nullptr,
				EEaseType::EaseOutQuad);
		};

	moveUI(m_hDestinationSpellMeter, screenCenter);
	moveUI(m_hDestinationSpellMeterBorder, screenCenter);
	moveUI(
		m_hDestinationSuccessFlame,
		{
			screenCenter.x,
			screenCenter.y + DESTINATION_SUCCESS_FLAME_OFFSET_Y
		});
	FadeOutCompletionSecondaryVisuals();
}

void CSpellMiniGame::FadeOutCompletionSecondaryVisuals()
{
	// Stop transient smoke from writing its own alpha while the common
	// completion fade controls it.
	m_TransientEffects.clear();

	const CHandle handles[] = {
		m_hPath,
		m_hIntroPathProgress,
		m_hPathProgress,
		m_hChaserPathProgress,
		m_hChaserCursor,
		m_hArrow,
		m_hCursor,
		m_hStartPadBackdrop,
		m_hStartPad,
		m_hDestinationSuccessDiamond
	};
	for (const CHandle handle : handles)
	{
		FadeOutUIHierarchy(
			handle,
			DESTINATION_SUCCESS_CENTER_MOVE_DURATION);
	}
	for (const BOOST_TRAIL_PARTICLE& particle : m_BoostTrailParticles)
	{
		FadeOutUIHierarchy(
			particle.Handle,
			DESTINATION_SUCCESS_CENTER_MOVE_DURATION);
	}
	auto fadeEffect = [this](const SUCCESS_EFFECT& effect)
		{
			const CHandle effectHandles[] = {
				effect.WispyHandle,
				effect.FireHandle,
				effect.CoreHandle,
				effect.SmokeHandle
			};
			for (const CHandle handle : effectHandles)
			{
				FadeOutUIHierarchy(
					handle,
					DESTINATION_SUCCESS_CENTER_MOVE_DURATION);
			}
		};

	fadeEffect(m_StartPadSuccessEffect);
	for (const BOOST_PAD& pad : m_BoostPads)
	{
		FadeOutUIHierarchy(
			pad.BackdropHandle,
			DESTINATION_SUCCESS_CENTER_MOVE_DURATION);
		FadeOutUIHierarchy(
			pad.Handle,
			DESTINATION_SUCCESS_CENTER_MOVE_DURATION);
		fadeEffect(pad.SuccessEffect);
	}
}

void CSpellMiniGame::PlayCompletionExitTransition()
{
	auto scaleToZero = [](CHandle handle)
		{
			auto* ui = E::CGameInstance::Get().
				GetGameObjectByHandleT<E::CUIObject>(handle);
			if (!ui)
				return;

			const _float startSize = ui->GetSize().x;
			if (auto* tween = ui->GetTweenCom())
			{
				tween->ClearTweens();
				tween->PlayTween(
					startSize,
					0.f,
					DESTINATION_SUCCESS_EXIT_DURATION,
					[handle](_float size)
					{
						if (auto* currentUI = E::CGameInstance::Get().
							GetGameObjectByHandleT<E::CUIObject>(handle))
						{
							currentUI->SetSize({ size, size });
							currentUI->CalcUICoord();
						}
					},
					nullptr,
					EEaseType::Linear);
			}
			else
			{
				ui->SetSize({ 0.f, 0.f });
				ui->CalcUICoord();
			}
		};

	scaleToZero(m_hDestinationSpellMeter);
	scaleToZero(m_hDestinationSpellMeterBorder);
	FadeOutUIHierarchy(
		m_hDestinationSuccessFlame,
		DESTINATION_SUCCESS_EXIT_DURATION);
}

void CSpellMiniGame::FadeOutUIHierarchy(
	CHandle rootHandle,
	_float duration)
{
	auto* root = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(rootHandle);
	if (!root || root->GetPendingDestroy())
		return;

	const auto childHandles = root->GetChildren();
	for (const CHandle childHandle : childHandles)
		FadeOutUIHierarchy(childHandle, duration);

	const _float startAlpha = root->GetAlpha();
	if (auto* tween = root->GetTweenCom())
	{
		tween->ClearTweens();
		tween->PlayTween(
			startAlpha,
			0.f,
			duration,
			[rootHandle](_float alpha)
			{
				if (auto* currentUI = E::CGameInstance::Get().
					GetGameObjectByHandleT<E::CUIObject>(rootHandle))
				{
					currentUI->SetAlpha(alpha);
				}
			},
			[rootHandle]()
			{
				if (auto* currentUI = E::CGameInstance::Get().
					GetGameObjectByHandleT<E::CUIObject>(rootHandle))
				{
					currentUI->SetAlpha(0.f);
					currentUI->SetActive(false);
					currentUI->SetVisible(false);
				}
			},
			EEaseType::Linear);
	}
	else
	{
		root->SetAlpha(0.f);
		root->SetActive(false);
		root->SetVisible(false);
	}
}

CSpellMiniGame::SUCCESS_EFFECT CSpellMiniGame::CreateMagicBurst(
	const _float2& position)
{
	SUCCESS_EFFECT effect{};
	const std::vector<CHandle> burstRoots =
		GET_SINGLE(UIManager)->LoadPrefab("MagicBurst");
	for (const CHandle handle : burstRoots)
	{
		auto* ui = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(handle);
		if (!ui)
			continue;

		ui->SetPos(position);
		ui->CalcUICoord();
		SetUIHierarchyInputLocked(handle);
		if (ui->GetUIInfo().Restag ==
			"TEX_VFX_T_FX_Smoke_Wispy2LiteSoftened01_D")
		{
			effect.WispyHandle = handle;
			ui->GetUIInfo().Weight = 910;
			ui->SetSize({ 88.f, 88.f });
			ui->SetAlpha(0.27f);
			ui->SetColor({ 2.65f, 2.65f, 2.65f });
		}
		else if (ui->GetUIInfo().Restag ==
			"TEX_UI_T_SpellMinigame_SpeedBurstDropDot")
		{
			effect.CoreHandle = handle;
			ui->SetResTag("TEX_T_FX_Stupify_Core_Center_D");
			ui->GetUIInfo().Weight = 914;
			ui->SetSize({ MAGIC_BURST_CORE_SIZE, MAGIC_BURST_CORE_SIZE });
			ui->SetAlpha(1.f);
			ui->SetColor({ 3.4f, 3.4f, 3.4f });
		}
		ui->CalcUICoord();
	}

	const std::vector<CHandle> fireRoots =
		GET_SINGLE(UIManager)->LoadPrefab("FireEffect");
	if (!fireRoots.empty())
	{
		effect.FireHandle = fireRoots.front();
		if (auto* fire = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(effect.FireHandle))
		{
			fire->SetPos({ position.x + 15.f, position.y - 30.f });
			fire->GetUIInfo().Weight = 912;
			fire->GetUIInfo().Rot = -15.f;
			fire->SetSize({ 32.f, 58.f });
			fire->SetAlpha(0.88f);
			fire->SetColor({ 0.9f, 1.65f, 2.35f });
			fire->CalcUICoord();
			SetUIHierarchyInputLocked(effect.FireHandle);
		}
	}

	const std::vector<CHandle> smokeRoots =
		GET_SINGLE(UIManager)->LoadPrefab("SmokeBurst");
	if (!smokeRoots.empty())
	{
		effect.SmokeHandle = smokeRoots.front();
		if (auto* smoke = E::CGameInstance::Get().
			GetGameObjectByHandleT<CEffectUI>(effect.SmokeHandle))
		{
			smoke->SetPos(position);
			smoke->SetSize({
				BOOST_SUCCESS_SMOKE_SIZE,
				BOOST_SUCCESS_SMOKE_SIZE
				});
			smoke->GetUIInfo().Weight = 916;
			smoke->SetAlpha(0.f);
			smoke->SetColor({ 0.f, 0.f, 0.f });
			smoke->CalcUICoord();
			SetUIHierarchyInputLocked(effect.SmokeHandle);
			SetUIHierarchyVisible(effect.SmokeHandle, false);
		}
	}

	return effect;
}

void CSpellMiniGame::PlayMagicBurst(
	const SUCCESS_EFFECT& effect,
	CHandle padHandle)
{
	E::CGameInstance::Get().GetSoundManager()->Play2D(
		"./Resources/SampleClient/Sound/UI/SpellClose.wav",
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::UI,
			.fVolume = 0.3f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});

	if (auto* wispy = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(effect.WispyHandle))
	{
		wispy->SetAlpha(0.27f);
	}
	if (auto* fire = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(effect.FireHandle))
	{
		fire->SetAlpha(0.88f);
	}
	if (auto* core = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(effect.CoreHandle))
	{
		core->SetAlpha(1.f);
		core->SetSize({ MAGIC_BURST_CORE_SIZE, MAGIC_BURST_CORE_SIZE });
		core->CalcUICoord();

		if (auto* tween = core->GetTweenCom())
		{
			tween->ClearTweens();
			tween->PlayTween(
				MAGIC_BURST_CORE_SIZE,
				MAGIC_BURST_CORE_PULSE_SIZE,
				MAGIC_BURST_CORE_GROW_TIME,
				[core](_float currentSize)
				{
					core->SetSize({ currentSize, currentSize });
					core->CalcUICoord();
				},
				nullptr,
				EEaseType::EaseOutQuad);

			tween->PlayTween(
				MAGIC_BURST_CORE_PULSE_SIZE,
				MAGIC_BURST_CORE_SIZE,
				MAGIC_BURST_CORE_RETURN_TIME,
				[core](_float currentSize)
				{
					core->SetSize({ currentSize, currentSize });
					core->CalcUICoord();
				},
				nullptr,
				EEaseType::EaseOutQuad,
				MAGIC_BURST_CORE_GROW_TIME);
		}
	}
	SetBoostPadHighlight(padHandle, true);
	CreateBoostSuccessSmoke(effect.SmokeHandle, padHandle);
	SetMagicBurstVisible(effect, true);
}

void CSpellMiniGame::CreateBoostSuccessSmoke(
	CHandle smokeHandle,
	CHandle padHandle,
	int weight,
	_float sizeScale,
	const _float3& color,
	_float duration)
{
	auto* pad = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(padHandle);
	auto* smoke = E::CGameInstance::Get().
		GetGameObjectByHandleT<CEffectUI>(smokeHandle);
	if (!pad || !smoke)
		return;

	std::erase_if(
		m_TransientEffects,
		[smokeHandle](const TRANSIENT_EFFECT& effect)
		{
			return effect.Handle == smokeHandle;
		});

	// The intro smoke uses a shorter lifetime, so speed up the flipbook as well
	// instead of cutting the animation off before its final frame.
	smoke->GetFlipInfo().Duration = duration;
	smoke->Restart();
	smoke->SetPos(pad->GetPos());
	smoke->SetSize({
		BOOST_SUCCESS_SMOKE_SIZE * sizeScale,
		BOOST_SUCCESS_SMOKE_SIZE * sizeScale
		});
	smoke->GetUIInfo().Weight = weight;
	smoke->SetAlpha(1.f);
	smoke->SetColor(color);
	smoke->SetInputLcok(true);
	smoke->SetActive(true);
	smoke->SetVisible(true);
	smoke->CalcUICoord();
	m_TransientEffects.push_back({
		.Handle = smokeHandle,
		.RemainingTime = duration,
		.Duration = duration
		});
}

void CSpellMiniGame::UpdateTransientEffects(_float fTimeDelta)
{
	for (auto iterator = m_TransientEffects.begin();
		iterator != m_TransientEffects.end();)
	{
		auto* effect = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(iterator->Handle);
		if (!effect || effect->GetPendingDestroy())
		{
			iterator = m_TransientEffects.erase(iterator);
			continue;
		}

		iterator->RemainingTime = std::max(
			0.f,
			iterator->RemainingTime - fTimeDelta);
		const _float lifeRatio = std::clamp(
			iterator->Duration > FLT_EPSILON ?
			iterator->RemainingTime / iterator->Duration : 0.f,
			0.f,
			1.f);
		effect->SetAlpha(lifeRatio);

		if (iterator->RemainingTime <= 0.f)
		{
			effect->SetAlpha(0.f);
			effect->SetActive(false);
			effect->SetVisible(false);
			iterator = m_TransientEffects.erase(iterator);
		}
		else
		{
			++iterator;
		}
	}
}

void CSpellMiniGame::ClearTransientEffects()
{
	for (const TRANSIENT_EFFECT& effect : m_TransientEffects)
	{
		if (auto* ui = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(effect.Handle))
		{
			if (!ui->GetPendingDestroy())
			{
				ui->SetAlpha(0.f);
				ui->SetActive(false);
				ui->SetVisible(false);
			}
		}
	}
	m_TransientEffects.clear();
}

void CSpellMiniGame::SetBoostPadHighlight(
	CHandle padHandle,
	_bool highlighted)
{
	if (auto* pad = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(padHandle))
	{
		pad->SetColor({ 0.f, 0.f, 0.f });
		pad->SetAlpha(highlighted ? 0.f : 1.f);
	}
}

void CSpellMiniGame::SetMagicBurstVisible(
	const SUCCESS_EFFECT& effect,
	_bool visible)
{
	SetUIHierarchyVisible(effect.WispyHandle, visible);
	SetUIHierarchyVisible(effect.FireHandle, visible);
	SetUIHierarchyVisible(effect.CoreHandle, visible);
}

void CSpellMiniGame::DestroyMagicBurst(SUCCESS_EFFECT& effect)
{
	DestroyUIHandle(effect.WispyHandle);
	DestroyUIHandle(effect.FireHandle);
	DestroyUIHandle(effect.CoreHandle);
	DestroyUIHandle(effect.SmokeHandle);
}

void CSpellMiniGame::SetUIHierarchyVisible(
	CHandle rootHandle,
	_bool visible)
{
	auto* root = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(rootHandle);
	if (!root)
		return;

	root->SetActive(visible);
	root->SetVisible(visible);
	for (const CHandle childHandle : root->GetChildren())
		SetUIHierarchyVisible(childHandle, visible);
}

void CSpellMiniGame::SetUIHierarchyInputLocked(CHandle rootHandle)
{
	auto* root = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(rootHandle);
	if (!root)
		return;

	root->SetInputLcok(true);
	for (const CHandle childHandle : root->GetChildren())
		SetUIHierarchyInputLocked(childHandle);
}

void CSpellMiniGame::DestroyUIHandle(CHandle& handle)
{
	if (auto* ui = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(handle))
	{
		ui->SetPendingDestroyCascade();
	}
	handle = {};
}

E::UPtr<CSpellMiniGame> CSpellMiniGame::Create()
{
	auto instance = E::ToUPtr(new CSpellMiniGame{});
	if (FAILED(instance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CSpellMiniGame");
		return nullptr;
	}
	return instance;
}

E::UPtr<E::CPrototype> CSpellMiniGame::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CSpellMiniGame{ *this });
	if (FAILED(instance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CSpellMiniGame");
		return nullptr;
	}
	return instance;
}

void CSpellMiniGame::Free()
{
	DestroyVisuals();
	CGameObject::Free();
}
