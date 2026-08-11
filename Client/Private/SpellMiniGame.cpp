#include "pch.h"
#include "SpellMiniGame.h"

#include "GameInstance.h"
#include "Level_Defines.h"
#include "SpellMeter.h"
#include "TextureUI.h"
#include "UI_Enums.h"

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
		SetStartPadVisible(false);
	}
	else if (m_eState == STATE::COMPLETED && aPressed)
	{
		ResetToStart();
		m_eState = STATE::RUNNING;
		ActivateBoost();
		SetStartPadVisible(false);
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
			const _float normalizedAlignment = std::clamp(
				(absoluteAlignment - ALIGNMENT_DEAD_ZONE) /
				(MAX_SPEED_ALIGNMENT - ALIGNMENT_DEAD_ZONE),
				0.f,
				1.f);
			const _float speedRatio =
				normalizedAlignment * normalizedAlignment *
				(3.f - 2.f * normalizedAlignment);
			const _float baseSpeed = std::lerp(
				MIN_MOVE_SPEED,
				MAX_MOVE_SPEED,
				speedRatio);
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
				m_fPathDistance + direction * speed * safeDelta,
				0.f,
				m_fTotalPathDistance);
		}

		if (m_fPathDistance >= m_fTotalPathDistance - 0.5f)
		{
			m_fPathDistance = m_fTotalPathDistance;
			m_eState = STATE::COMPLETED;
			ResetChaser();
			SetCursorVisible(false);
			SetStartPadVisible(true);
		}
	}

	UpdateChaser(safeDelta);

	UpdateArrowVisual(
		EvaluatePosition(m_fPathDistance),
		facingDirection);
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

void CSpellMiniGame::ResetToStart()
{
	m_eState = STATE::WAITING;
	m_fPathDistance = 0.f;
	m_fBoostTimeRemaining = 0.f;
	m_vLastFacingDirection = EvaluateForward(0.f);
	ResetChaser();
	ResetBoostPads();
	SetCursorVisible(true);
	SetStartPadVisible(true);
	if (auto* pathProgress = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(m_hPathProgress))
	{
		pathProgress->SetPathProgress(0.f);
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

	m_hPath = createTexture(
		"SpellMiniGame_IncendioPath",
		"TEX_UI_T_SU_Incendio_Path",
		screenCenter,
		{ m_fPathSize, m_fPathSize },
		900,
		{ 0.82f, 0.68f, 0.22f });
	m_hPathProgress = createTexture(
		"SpellMiniGame_IncendioPathProgress",
		"TEX_UI_T_SU_Incendio_Path",
		screenCenter,
		{ m_fPathSize, m_fPathSize },
		901,
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
		902,
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
		906,
		{ 0.f, 0.f, 0.f });
	m_hArrow = createTexture(
		"SpellMiniGame_CursorArrow",
		"TEX_UI_T_SU_CursorArrow",
		startPosition,
		{ CURSOR_ARROW_SIZE, CURSOR_ARROW_SIZE },
		906,
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
	m_hStartPad = createTexture(
		"SpellMiniGame_StartPad",
		"TEX_UI_T_cbi_button_Abutton",
		startPosition,
		{ BOOST_PAD_ICON_SIZE, BOOST_PAD_ICON_SIZE },
		907,
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
		const std::string backdropTag =
			std::string{ padDesc.ObjectTag } + "_Backdrop";
		const CHandle backdropHandle = createTexture(
			backdropTag,
			"TEX_UI_T_SU_ButtonCalloutRing",
			EvaluatePosition(pathDistance),
			{ BOOST_PAD_BACKDROP_SIZE, BOOST_PAD_BACKDROP_SIZE },
			903,
			{ 0.f, 0.f, 0.f });
		const CHandle handle = createTexture(
			padDesc.ObjectTag,
			padDesc.ResourceTag,
			EvaluatePosition(pathDistance),
			{ BOOST_PAD_ICON_SIZE, BOOST_PAD_ICON_SIZE },
			904,
			{ 0.f, 0.f, 0.f });
		m_BoostPads.push_back({
			.PathDistance = pathDistance,
			.KeyCode = padDesc.KeyCode,
			.BackdropHandle = backdropHandle,
			.Handle = handle
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
					GetGameObjectByHandleT<E::CUIObject>(pad.Handle) != nullptr;
			});

	return boostPadsCreated && E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hPath) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hPathProgress) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hChaserPathProgress) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hChaserCursor) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<CSpellMeter>(m_hDestinationSpellMeter) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hDestinationSpellMeterBorder) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hArrow) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hCursor) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hStartPadBackdrop) &&
		E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hStartPad);
}

void CSpellMiniGame::DestroyVisuals()
{
	for (BOOST_PAD& pad : m_BoostPads)
	{
		DestroyUIHandle(pad.BackdropHandle);
		DestroyUIHandle(pad.Handle);
	}
	m_BoostPads.clear();
	DestroyUIHandle(m_hStartPad);
	DestroyUIHandle(m_hStartPadBackdrop);
	DestroyUIHandle(m_hArrow);
	DestroyUIHandle(m_hCursor);
	DestroyUIHandle(m_hChaserCursor);
	DestroyUIHandle(m_hDestinationSpellMeterBorder);
	DestroyUIHandle(m_hDestinationSpellMeter);
	DestroyUIHandle(m_hChaserPathProgress);
	DestroyUIHandle(m_hPathProgress);
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
		if (auto* backdropUI = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(pad.BackdropHandle))
		{
			backdropUI->SetActive(false);
			backdropUI->SetVisible(false);
		}
		if (auto* padUI = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(pad.Handle))
		{
			padUI->SetActive(false);
			padUI->SetVisible(false);
		}
		break;
	}
}

void CSpellMiniGame::ActivateBoost()
{
	m_fBoostTimeRemaining = BOOST_DURATION;
}

void CSpellMiniGame::UpdateChaser(_float fTimeDelta)
{
	if (m_eState != STATE::RUNNING || m_BoostPads.empty())
		return;

	if (!m_bChaserActive)
	{
		if (m_fPathDistance < m_BoostPads.front().PathDistance)
			return;

		m_bChaserActive = true;
		m_fChaserPathDistance = 0.f;
		SetChaserVisible(true);
	}

	m_fChaserPathDistance = std::min(
		m_fTotalPathDistance,
		m_fChaserPathDistance + CHASER_MOVE_SPEED * fTimeDelta);

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
		if (auto* backdropUI = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(pad.BackdropHandle))
		{
			backdropUI->SetActive(true);
			backdropUI->SetVisible(true);
		}
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
	if (auto* startPadBackdrop = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(m_hStartPadBackdrop))
	{
		startPadBackdrop->SetActive(visible);
		startPadBackdrop->SetVisible(visible);
	}

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
