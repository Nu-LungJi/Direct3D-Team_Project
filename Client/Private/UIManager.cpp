#include "pch.h"
#include "GameInstance.h"
#include "UIManager.h"
#include "TextureUI.h"
#include "EffectUI.h"
#include "TextBox.h"
#include "Button.h"
#include "GeneralButton.h"
#include <fstream>
#include "LevelLogo.h"
#include "LevelLoading.h"
#include "SpellMeter.h"
#include "HPBar.h"
#include "Level_Defines.h"
#include "MiniMap.h"
#include "UIController.h"
#include "GameOverMask.h"
#include "VideoObject.h"
#include "Monster.h"
#include "Player.h"
#include "SoundManager.h"

NS_USING(Client)

namespace
{
	constexpr const _char* ASSIO_UI_START_SOUND_PATH =
		"./Resources/SampleClient/Sound/AccioActivity/UI/AccioUI_Start.wav";
	constexpr const _char* ASSIO_UI_END_SOUND_PATH =
		"./Resources/SampleClient/Sound/AccioActivity/UI/AccioUI_End.wav";

	void PlayAssioUISound(const _char* pSoundPath, _float fVolume)
	{
		if (auto* pSoundManager = E::CGameInstance::Get().GetSoundManager())
		{
			pSoundManager->Play2D(
				pSoundPath,
				SOUND_PLAY_DESC{
					.sBusID = SOUND_BUS::UI,
					.fVolume = fVolume,
					.fPitch = 1.f,
					.iPriority = 70,
					.bLoop = false
				});
		}
	}

	void SetRenderGroupRecursive(CHandle handle, E::RENDERGROUP renderGroup)
	{
		auto* ui = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUIObject>(handle);
		if (!ui)
			return;
		ui->SetRenderGroupOverride(renderGroup);
		for (const CHandle child : ui->GetChildren())
			SetRenderGroupRecursive(child, renderGroup);
	}

	constexpr _float DIALOGUE_FONT_SCALE = 1.f;
	constexpr _float DIALOGUE_HOLD_TIME = 5.f;
	constexpr _float DIALOGUE_FADE_IN_TIME = 0.15f;
	constexpr _float DIALOGUE_FADE_OUT_TIME = 0.25f;
	constexpr _float DIALOGUE_BOTTOM_MARGIN = 100.f;
	constexpr _float DIALOGUE_ROW_INTERVAL = 28.f;
	constexpr _float DIALOGUE_BACKGROUND_HEIGHT = 50.f;
	constexpr _float DIALOGUE_SIDE_PADDING = 31.f;
	constexpr _float DIALOGUE_TEXT_GAP = 11.f;
	constexpr _float DIALOGUE_TEXT_Y_OFFSET = -13.f;
	constexpr _float DIALOGUE_MIN_WIDTH = 0.f;
	constexpr _float DIALOGUE_MAX_WIDTH = 1100.f;
	constexpr size_t DIALOGUE_MAX_COUNT = 6u;

	constexpr _float NPC_SPEECH_FONT_SCALE = 1.f;
	constexpr _float NPC_SPEECH_DEFAULT_DURATION = 5.f;
	constexpr _float NPC_SPEECH_FADE_IN_TIME = 0.15f;
	constexpr _float NPC_SPEECH_FADE_OUT_TIME = 0.25f;
	constexpr _float NPC_SPEECH_BACKGROUND_HEIGHT = 50.f;
	constexpr _float NPC_SPEECH_SIDE_PADDING = 28.f;
	constexpr _float NPC_SPEECH_TEXT_Y_OFFSET = -13.f;
	constexpr _float NPC_SPEECH_NEAR_DISTANCE = 3.f;
	constexpr _float NPC_SPEECH_FAR_DISTANCE = 20.f;
	constexpr _float NPC_SPEECH_MAX_SCALE = 1.15f;
	constexpr _float NPC_SPEECH_MIN_SCALE = 0.55f;
	constexpr _float NPC_SPEECH_SCALE_SMOOTH_SPEED = 10.f;

	TEXT_ALIGN LoadTextAlignmentCompatible(
		const nlohmann::ordered_json& obj)
	{
		const uint32_t alignment = obj.value(
			"TextAlignment",
			static_cast<uint32_t>(TEXT_ALIGN::LEFT));
		if (alignment > static_cast<uint32_t>(TEXT_ALIGN::RIGHT))
			return TEXT_ALIGN::LEFT;

		return static_cast<TEXT_ALIGN>(alignment);
	}

	TEXT_FONT_TYPE LoadTextFontTypeCompatible(
		const nlohmann::ordered_json& obj)
	{
		if (obj.contains("FontType"))
		{
			const uint32_t fontType = obj.value("FontType", 0u);
			const auto loadedType = static_cast<TEXT_FONT_TYPE>(fontType);
			switch (loadedType)
			{
			case TEXT_FONT_TYPE::DEFAULT:
			case TEXT_FONT_TYPE::BLUE_FOREST_BOLD_20:
			case TEXT_FONT_TYPE::BLUE_FOREST_BOLD_32:
			case TEXT_FONT_TYPE::PRETENDARD_64:
			case TEXT_FONT_TYPE::HAKGYOANSIM_PUZZLE_OUTLINE_25:
				return loadedType;
			default:
				break;
			}
		}

		const std::string legacyName = obj.value("Name", std::string{});
		if (legacyName == "BF20")
			return TEXT_FONT_TYPE::BLUE_FOREST_BOLD_20;
		if (legacyName == "BF32")
			return TEXT_FONT_TYPE::BLUE_FOREST_BOLD_32;
		if (legacyName == "64px")
			return TEXT_FONT_TYPE::PRETENDARD_64;
		return TEXT_FONT_TYPE::DEFAULT;
	}

	void LoadFlipInfoCompatible(
		const nlohmann::ordered_json& obj,
		FLIP_INFO& flipInfo)
	{
		flipInfo.cellsize = obj.value("CellSize", 4096u);
		flipInfo.TotalFrame = std::max(
			1u,
			obj.value("TotalFrame", 64u));
		flipInfo.Padding = obj.value("Padding", 2.f);
		flipInfo.Duration = obj.value("Duration", 1.5f);

		// Legacy files only stored TotalFrame and assumed a square sheet.
		const uint32_t legacyGridSize = std::max(
			1u,
			static_cast<uint32_t>(std::round(
				std::sqrt(static_cast<_float>(flipInfo.TotalFrame)))));
		flipInfo.Columns = std::max(
			1u,
			obj.value("Columns", legacyGridSize));
		flipInfo.Rows = std::max(
			1u,
			obj.value("Rows", legacyGridSize));
	}

	CUIObject* FindUIByNameRecursive(
		const std::vector<CHandle>& roots,
		std::string_view targetName)
	{
		std::vector<CHandle> pending = roots;
		for (size_t index = 0; index < pending.size(); ++index)
		{
			auto* ui = GetSafeUI(pending[index]);
			if (!ui)
				continue;

			if (std::string_view(ui->GetName()) == targetName)
				return ui;

			const auto& children = ui->GetChildren();
			pending.insert(pending.end(), children.begin(), children.end());
		}
		return nullptr;
	}
}

UIManager::~UIManager()
{
	MFShutdown();
}

_bool UIManager::IsSpellUnlocked(SPELL_TYPE spellType) const
{
	const size_t index = static_cast<size_t>(spellType);
	return index < m_SpellUnlockStates.size() && m_SpellUnlockStates[index];
}

void UIManager::SetSpellUnlocked(SPELL_TYPE spellType, _bool unlocked)
{
	const size_t index = static_cast<size_t>(spellType);
	if (index >= m_SpellUnlockStates.size())
		return;

	m_SpellUnlockStates[index] = unlocked;
	if (!m_UIController)
		return;

	if (auto* controller = E::CGameInstance::Get().
		GetGameObjectByHandleT<CUIController>(*m_UIController))
	{
		controller->SetSpellUnlocked(spellType, unlocked);
	}
}

uint32_t UIManager::GetSavedSpellSlot(uint32_t slotNumber) const
{
	if (slotNumber < 1u || slotNumber > m_SavedSpellSlots.size())
		return ETOUI(SPELL_TYPE::NONE);
	return m_SavedSpellSlots[slotNumber - 1u];
}

void UIManager::SaveSpellSlot(uint32_t slotNumber, uint32_t spellType)
{
	if (slotNumber < 1u || slotNumber > m_SavedSpellSlots.size())
		return;
	m_SavedSpellSlots[slotNumber - 1u] = spellType;
	m_bSpellSlotsInitialized = true;
}

void UIManager::Update(_float fTimeDelta)
{
	UpdateWandShopWorldBillboard();
	UpdateWandShopWorldMousePosition();
	UpdateActiveButtons();
	UpdateDialoguePopups(fTimeDelta);
	UpdateNPCSpeechBubbles(fTimeDelta);
	UpdateDialogueChoiceUI();
	UpdateRaceStartTimer(fTimeDelta);
	UpdateRaceMiniGame(fTimeDelta);
	UpdateAssioMiniGame(fTimeDelta);
	m_WandShop.Update(*this, fTimeDelta);
}

void UIManager::UpdateWandShopWorldBillboard()
{
	if (!m_bWandShopWorldMode)
		return;

	auto* camera = E::CGameInstance::Get().GetActiveCamera();
	if (!camera)
		return;

	const _vector panelPosition = XMLoadFloat3(&m_vWandShopPanelWorldPosition);
	_vector panelLook = camera->GetTransform().GetLoadedPostion() - panelPosition;
	// Keep the shop panel upright while it turns toward the active gameplay or
	// cinematic camera.  A nearly vertical camera angle retains the last pose.
	panelLook = XMVectorSetY(panelLook, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(panelLook)) <= 0.0001f)
		return;

	panelLook = XMVector3Normalize(panelLook);
	const _vector worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	const _vector panelRight = XMVector3Normalize(
		XMVector3Cross(worldUp, panelLook));
	const _matrix panelPose{
		XMVectorSetW(panelRight, 0.f),
		worldUp,
		XMVectorSetW(panelLook, 0.f),
		XMVectorSetW(panelPosition, 1.f)
	};
	const _matrix panelWorld = XMMatrixScaling(
		m_vWandShopPanelWorldScale.x,
		m_vWandShopPanelWorldScale.y,
		1.f) * panelPose;

	XMStoreFloat4x4(&m_WandShopPanelWorld, panelWorld);
	E::CGameInstance::Get().SetUI3DPanel(
		m_WandShopPanelWorld, true, false);
}

void UIManager::AssioMiniGameStart(_bool bPlayerStarts)
{
	ClearAssioMiniGameUI();
	// 미니게임 UI를 만들기 전에 기존 UI만 복원 목록에 저장해 숨긴다.
	// 이후 생성되는 Coat UI는 전체 UI FadeOut 대상에 포함되지 않는다.
	PlayFadeOutAll2DUI(0.f, 0.35f);
	m_AssioMiniGameRoots = LoadPrefab("Coat");

	auto StoreHandle = [this](
		std::string_view name,
		std::optional<CHandle>& destination)
	{
		if (auto* ui = FindUIByNameRecursive(m_AssioMiniGameRoots, name))
			destination = ui->GetHandle();
	};

	StoreHandle("ScoreBoard", m_hAssioScoreBoard);
	StoreHandle("playerScore", m_hAssioPlayerScoreRoot);
	StoreHandle("NpcScore", m_hAssioNpcScoreRoot);
	StoreHandle("CenterScore", m_hAssioCenterScore);
	StoreHandle("TurnTitle", m_hAssioTurnTitle);

	if (m_hAssioPlayerScoreRoot)
	{
		const std::vector<CHandle> playerRoot{ *m_hAssioPlayerScoreRoot };
		if (auto* score = FindUIByNameRecursive(playerRoot, "Score"))
			m_hAssioPlayerScoreText = score->GetHandle();
		if (auto* frame = FindUIByNameRecursive(playerRoot, "frame"))
			m_hAssioPlayerFrame = frame->GetHandle();
	}
	if (m_hAssioNpcScoreRoot)
	{
		const std::vector<CHandle> npcRoot{ *m_hAssioNpcScoreRoot };
		if (auto* score = FindUIByNameRecursive(npcRoot, "Score"))
			m_hAssioNpcScoreText = score->GetHandle();
		if (auto* frame = FindUIByNameRecursive(npcRoot, "frame"))
			m_hAssioNpcFrame = frame->GetHandle();
	}
	if (m_hAssioCenterScore)
	{
		const std::vector<CHandle> centerRoot{ *m_hAssioCenterScore };
		if (auto* text = FindUIByNameRecursive(centerRoot, "64px"))
			m_hAssioCenterScoreText = text->GetHandle();

		if (auto* center = GetSafeUI(*m_hAssioCenterScore))
		{
			m_AssioCenterScoreBasePosition = center->GetPos();
			m_fAssioCenterScoreBaseScale = center->GetScaleRatio();
			m_fAssioCenterScoreBaseAlpha = center->GetAlpha();
			const CHandle centerHandle = center->GetHandle();
			center->SetAlpha(0.f);
			center->SetScaleRatio(0.f);
			center->CalcUICoord();
			center->Appear = [centerHandle](CUIObject*)
			{
				if (auto* current = GetSafeUI(centerHandle))
				{
					current->SetAlpha(0.f);
					current->SetScaleRatio(0.f);
					current->CalcUICoord();
				}
			};
		}
	}

	const _bool valid = m_hAssioScoreBoard.has_value() &&
		m_hAssioPlayerScoreRoot.has_value() &&
		m_hAssioNpcScoreRoot.has_value() &&
		m_hAssioPlayerScoreText.has_value() &&
		m_hAssioNpcScoreText.has_value() &&
		m_hAssioPlayerFrame.has_value() &&
		m_hAssioNpcFrame.has_value() &&
		m_hAssioCenterScore.has_value() &&
		m_hAssioCenterScoreText.has_value() &&
		m_hAssioTurnTitle.has_value();
	if (!valid)
	{
		ClearAssioMiniGameUI();
		PlayFadeInAll2DUI(0.f, 0.35f);
		return;
	}

	auto* playerRoot = GetSafeUI(*m_hAssioPlayerScoreRoot);
	auto* npcRoot = GetSafeUI(*m_hAssioNpcScoreRoot);
	auto* playerFrame = GetSafeUI(*m_hAssioPlayerFrame);
	auto* npcFrame = GetSafeUI(*m_hAssioNpcFrame);
	auto* turnTitle = GetSafeUI(*m_hAssioTurnTitle);
	if (!playerRoot || !npcRoot || !playerFrame || !npcFrame || !turnTitle)
	{
		ClearAssioMiniGameUI();
		PlayFadeInAll2DUI(0.f, 0.35f);
		return;
	}

	// Coat 프리팹에서 현재 턴/대기 턴의 원래 배치를 보관한다.
	m_AssioActiveTurnPosition = playerRoot->GetPos();
	m_AssioInactiveTurnPosition = npcRoot->GetPos();
	m_fAssioActiveTurnScale = playerRoot->GetScaleRatio();
	m_fAssioInactiveTurnScale = npcRoot->GetScaleRatio();
	m_fAssioActiveTurnAlpha = playerRoot->GetAlpha();
	m_fAssioInactiveTurnAlpha = npcRoot->GetAlpha();
	// frame은 자식이므로 실제 Alpha가 아니라 AlphaRatio를 보간해야 한다.
	// 부모의 CalcUICoord()가 실제 Alpha를 부모 Alpha * AlphaRatio로 계산한다.
	m_fAssioActiveFrameAlpha = playerFrame->GetAlphaRatio();
	m_fAssioInactiveFrameAlpha = npcFrame->GetAlphaRatio();
	m_fAssioTurnTitleBaseAlpha = turnTitle->GetAlpha();
	m_bAssioCurrentTurnIsPlayer = bPlayerStarts;
	m_bAssioTurnTitleFadeInStarted = false;
	m_bAssioTurnTitleWasAlreadyHidden = false;
	auto ApplyInitialTurnLayout = [this](
		CUIObject* root, CUIObject* frame, _bool bActive)
	{
		root->SetPos(bActive ?
			m_AssioActiveTurnPosition : m_AssioInactiveTurnPosition);
		root->SetScaleRatio(bActive ?
			m_fAssioActiveTurnScale : m_fAssioInactiveTurnScale);
		root->SetAlpha(bActive ?
			m_fAssioActiveTurnAlpha : m_fAssioInactiveTurnAlpha);
		frame->SetAlphaRatio(bActive ?
			m_fAssioActiveFrameAlpha : m_fAssioInactiveFrameAlpha);
		root->CalcUICoord();
	};
	ApplyInitialTurnLayout(playerRoot, playerFrame, bPlayerStarts);
	ApplyInitialTurnLayout(npcRoot, npcFrame, !bPlayerStarts);
	if (auto* title = dynamic_cast<CTextBox*>(turnTitle))
	{
		title->SetwText(bPlayerStarts ?
			L"이솝 샤프의 턴" : L"저스티스 훈의 턴");
	}

	m_iAssioPlayerScore = 0;
	m_iAssioNpcScore = 0;
	m_iAssioPendingScore = 0;
	m_iAssioPendingPlayerScore = 0;
	m_iAssioPendingNpcScore = 0;
	m_fAssioScorePhaseElapsed = 0.f;
	m_bAssioFinalScore = false;
	m_eAssioScorePhase = ASSIO_SCORE_PHASE::NONE;
	m_bAssioMiniGameActive = true;
	PlayAssioUISound(ASSIO_UI_START_SOUND_PATH, 0.75f);

	if (auto* text = dynamic_cast<CTextBox*>(
		GetSafeUI(*m_hAssioPlayerScoreText)))
		text->SetwText(L"0");
	if (auto* text = dynamic_cast<CTextBox*>(
		GetSafeUI(*m_hAssioNpcScoreText)))
		text->SetwText(L"0");

	PlayRaceRootsFadeIn({
		*m_hAssioScoreBoard,
		*m_hAssioPlayerScoreRoot,
		*m_hAssioNpcScoreRoot,
		*m_hAssioTurnTitle
	}, 0.35f);
}

void UIManager::AssioMiniGameFinish()
{
	// Coat UI는 사라지게 하고, 시작 시 숨겼던 기존 UI만 다시 복원한다.
	ClearAssioMiniGameUI(false);
	PlayFadeInAll2DUI(0.f, 0.5f);
}

void UIManager::TurnTitleFadeOut(float playtime)
{
	if (!m_hAssioTurnTitle)
		return;

	const CHandle titleHandle = *m_hAssioTurnTitle;
	auto* title = GetSafeUI(titleHandle);
	if (!title)
		return;

	const _float startAlpha = title->GetAlpha();
	if (auto* tween = title->GetTweenCom())
	{
		tween->ClearTweens();
		tween->PlayTween(
			startAlpha,
			0.f,
			std::max(0.f, playtime),
			[titleHandle](_float value)
			{
				if (auto* current = GetSafeUI(titleHandle))
					current->SetAlpha(value);
			}, nullptr, EEaseType::EaseOutQuad);
	}
	else
	{
		title->SetAlpha(0.f);
	}
}

_bool UIManager::AddScore(
	int iTurnScore,
	int iPlayerTotalScore,
	int iNpcTotalScore,
	_bool bPlayerTurn,
	_bool bFinalScore)
{
	if (!m_bAssioMiniGameActive ||
		m_eAssioScorePhase != ASSIO_SCORE_PHASE::NONE ||
		!m_hAssioCenterScore || !m_hAssioCenterScoreText)
	{
		return false;
	}
	m_iAssioPendingScore = std::max(0, iTurnScore);
	m_iAssioPendingPlayerScore = std::max(0, iPlayerTotalScore);
	m_iAssioPendingNpcScore = std::max(0, iNpcTotalScore);
	m_bAssioCurrentTurnIsPlayer = bPlayerTurn;
	if (!ResolveAssioCurrentTurn())
	{
		m_iAssioPendingScore = 0;
		m_iAssioPendingPlayerScore = 0;
		m_iAssioPendingNpcScore = 0;
		return false;
	}

	auto* center = GetSafeUI(*m_hAssioCenterScore);
	auto* centerText = dynamic_cast<CTextBox*>(
		GetSafeUI(*m_hAssioCenterScoreText));
	if (!center || !centerText)
	{
		m_iAssioPendingScore = 0;
		m_iAssioPendingPlayerScore = 0;
		m_iAssioPendingNpcScore = 0;
		m_hAssioTargetScoreText.reset();
		return false;
	}
	m_bAssioFinalScore = bFinalScore;

	centerText->SetwText(std::to_wstring(m_iAssioPendingScore));
	center->SetPos(m_AssioCenterScoreBasePosition);
	center->SetScaleRatio(0.f);
	center->SetAlpha(0.f);
	center->CalcUICoord();

	m_AssioCenterScoreMoveStart = m_AssioCenterScoreBasePosition;
	m_fAssioScorePhaseElapsed = 0.f;
	m_eAssioScorePhase = ASSIO_SCORE_PHASE::APPEAR;
	return true;
}

_bool UIManager::ResolveAssioCurrentTurn()
{
	if (!m_hAssioPlayerScoreRoot || !m_hAssioNpcScoreRoot ||
		!m_hAssioPlayerFrame || !m_hAssioNpcFrame ||
		!m_hAssioPlayerScoreText || !m_hAssioNpcScoreText)
	{
		return false;
	}

	auto* playerRoot = GetSafeUI(*m_hAssioPlayerScoreRoot);
	auto* npcRoot = GetSafeUI(*m_hAssioNpcScoreRoot);
	auto* playerFrame = GetSafeUI(*m_hAssioPlayerFrame);
	auto* npcFrame = GetSafeUI(*m_hAssioNpcFrame);
	if (!playerRoot || !npcRoot || !playerFrame || !npcFrame)
		return false;

	const _bool targetPlayer = m_bAssioCurrentTurnIsPlayer;

	m_bAssioTargetIsPlayer = targetPlayer;
	m_hAssioTargetScoreText = targetPlayer ?
		m_hAssioPlayerScoreText : m_hAssioNpcScoreText;
	if (!m_hAssioTargetScoreText)
		return false;

	if (auto* target = GetSafeUI(*m_hAssioTargetScoreText))
	{
		const int previewScore = targetPlayer ?
			m_iAssioPendingPlayerScore : m_iAssioPendingNpcScore;
		const std::wstring previewText = std::to_wstring(previewScore);
		const _float textScale = target->GetUIInfo().SizeX *
			target->GetScaleRatio();
		const _float2 textSize = E::CGameInstance::Get().FontMeasureString(
			"Pretendard", previewText.c_str());
		m_AssioCenterScoreMoveTarget = target->GetPos();
		// Score is right-aligned, so its anchor is the right edge rather than
		// the visual center of the number.
		m_AssioCenterScoreMoveTarget.x -= textSize.x * textScale * 0.5f;
		m_AssioCenterScoreMoveTarget.y += 6.f;
		return true;
	}
	return false;
}

void UIManager::UpdateAssioMiniGame(_float fTimeDelta)
{
	if (!m_bAssioMiniGameActive ||
		m_eAssioScorePhase == ASSIO_SCORE_PHASE::NONE)
	{
		return;
	}

	m_fAssioScorePhaseElapsed += std::max(0.f, fTimeDelta);
	if (m_eAssioScorePhase == ASSIO_SCORE_PHASE::RESULT_COAT_FADE_OUT)
	{
		if (m_fAssioScorePhaseElapsed >= 0.3f)
			LoadAssioResult();
		return;
	}
	if (m_eAssioScorePhase == ASSIO_SCORE_PHASE::RESULT_HOLD)
	{
		if (m_fAssioScorePhaseElapsed >= 3.f)
		{
			for (const CHandle root : m_AssioResultRoots)
			{
				if (GetSafeUI(root))
					PlayFadeOutDelete(root, 0.f, 0.3f);
			}
			m_AssioResultRoots.clear();
			m_fAssioScorePhaseElapsed = 0.f;
			m_eAssioScorePhase = ASSIO_SCORE_PHASE::RESULT_FADE_OUT;
		}
		return;
	}
	if (m_eAssioScorePhase == ASSIO_SCORE_PHASE::RESULT_FADE_OUT)
	{
		if (m_fAssioScorePhaseElapsed >= 0.3f)
		{
			m_fAssioScorePhaseElapsed = 0.f;
			m_bAssioFinalScore = false;
			m_bAssioMiniGameActive = false;
			m_eAssioScorePhase = ASSIO_SCORE_PHASE::NONE;
			PlayFadeInAll2DUI(0.f, 0.5f);
		}
		return;
	}

	if (!m_hAssioCenterScore || !m_hAssioTargetScoreText)
		return;

	auto* center = GetSafeUI(*m_hAssioCenterScore);
	auto* targetScore = GetSafeUI(*m_hAssioTargetScoreText);
	if (!center || !targetScore)
	{
		m_eAssioScorePhase = ASSIO_SCORE_PHASE::NONE;
		return;
	}

	if (m_eAssioScorePhase == ASSIO_SCORE_PHASE::IMPACT_HOLD)
	{
		// Normally the ring FadeOut completion callback advances immediately.
		// Keep only a fallback in case the transient effect could not be created.
		constexpr _float IMPACT_FALLBACK_DURATION = 0.8f;
		if (m_fAssioScorePhaseElapsed < IMPACT_FALLBACK_DURATION)
			return;

		CompleteAssioScoreImpact();
		return;
	}

	if (m_eAssioScorePhase == ASSIO_SCORE_PHASE::TURN_CHANGE)
	{
		auto* playerRoot = GetSafeUI(*m_hAssioPlayerScoreRoot);
		auto* npcRoot = GetSafeUI(*m_hAssioNpcScoreRoot);
		auto* playerFrame = GetSafeUI(*m_hAssioPlayerFrame);
		auto* npcFrame = GetSafeUI(*m_hAssioNpcFrame);
		if (!playerRoot || !npcRoot || !playerFrame || !npcFrame)
		{
			m_eAssioScorePhase = ASSIO_SCORE_PHASE::NONE;
			return;
		}

		constexpr _float TITLE_FADE_OUT_DURATION = 0.2f;
		constexpr _float TURN_CHANGE_DURATION = 0.65f;
		const _bool nextTurnIsPlayer = !m_bAssioCurrentTurnIsPlayer;
		const _float titleFadeInDelay =
			m_bAssioTurnTitleWasAlreadyHidden ?
			0.f : TITLE_FADE_OUT_DURATION;
		if (!m_bAssioTurnTitleFadeInStarted &&
			m_fAssioScorePhaseElapsed >= titleFadeInDelay)
		{
			m_bAssioTurnTitleFadeInStarted = true;
			if (auto* title = dynamic_cast<CTextBox*>(
				GetSafeUI(*m_hAssioTurnTitle)))
			{
				title->SetwText(nextTurnIsPlayer ?
					L"이솝 샤프의 턴" : L"저스티스 훈의 턴");
				title->SetAlpha(0.f);
				if (auto* tween = title->GetTweenCom())
				{
					const CHandle titleHandle = title->GetHandle();
					tween->ClearTweens();
					tween->PlayTween(
						0.f, m_fAssioTurnTitleBaseAlpha, 0.3f,
						[titleHandle](_float value)
						{
							if (auto* current = GetSafeUI(titleHandle))
								current->SetAlpha(value);
						}, nullptr, EEaseType::EaseOutQuad);
				}
				else
				{
					title->SetAlpha(m_fAssioTurnTitleBaseAlpha);
				}
			}
		}

		const _float ratio = std::clamp(
			m_fAssioScorePhaseElapsed / TURN_CHANGE_DURATION, 0.f, 1.f);
		const _float eased = ratio * ratio * (3.f - 2.f * ratio);
		auto ApplyTurnLayout = [this, eased](
			CUIObject* root, CUIObject* frame, _bool becomingActive)
		{
			const _float2 startPosition = becomingActive ?
				m_AssioInactiveTurnPosition : m_AssioActiveTurnPosition;
			const _float2 targetPosition = becomingActive ?
				m_AssioActiveTurnPosition : m_AssioInactiveTurnPosition;
			const _float startScale = becomingActive ?
				m_fAssioInactiveTurnScale : m_fAssioActiveTurnScale;
			const _float targetScale = becomingActive ?
				m_fAssioActiveTurnScale : m_fAssioInactiveTurnScale;
			const _float startAlpha = becomingActive ?
				m_fAssioInactiveTurnAlpha : m_fAssioActiveTurnAlpha;
			const _float targetAlpha = becomingActive ?
				m_fAssioActiveTurnAlpha : m_fAssioInactiveTurnAlpha;
			const _float startFrameAlpha = becomingActive ?
				m_fAssioInactiveFrameAlpha : m_fAssioActiveFrameAlpha;
			const _float targetFrameAlpha = becomingActive ?
				m_fAssioActiveFrameAlpha : m_fAssioInactiveFrameAlpha;

			root->SetPos({
				std::lerp(startPosition.x, targetPosition.x, eased),
				std::lerp(startPosition.y, targetPosition.y, eased)
			});
			root->SetScaleRatio(std::lerp(startScale, targetScale, eased));
			root->SetAlpha(std::lerp(startAlpha, targetAlpha, eased));
			frame->SetAlphaRatio(std::lerp(
				startFrameAlpha, targetFrameAlpha, eased));
			root->CalcUICoord();
		};

		ApplyTurnLayout(playerRoot, playerFrame, nextTurnIsPlayer);
		ApplyTurnLayout(npcRoot, npcFrame, !nextTurnIsPlayer);
		if (ratio < 1.f)
			return;

		m_bAssioCurrentTurnIsPlayer = nextTurnIsPlayer;
		m_bAssioTurnTitleFadeInStarted = false;
		m_hAssioTargetScoreText.reset();
		m_fAssioScorePhaseElapsed = 0.f;
		m_eAssioScorePhase = ASSIO_SCORE_PHASE::NONE;
		return;
	}

	if (m_eAssioScorePhase == ASSIO_SCORE_PHASE::APPEAR)
	{
		constexpr _float APPEAR_DURATION = 0.4f;
		const _float ratio = std::clamp(
			m_fAssioScorePhaseElapsed / APPEAR_DURATION, 0.f, 1.f);
		const _float eased = 1.f - std::pow(1.f - ratio, 3.f);
		center->SetScaleRatio(m_fAssioCenterScoreBaseScale * eased);
		center->SetAlpha(m_fAssioCenterScoreBaseAlpha * ratio);
		center->CalcUICoord();
		if (ratio >= 1.f)
		{
			m_fAssioScorePhaseElapsed = 0.f;
			m_eAssioScorePhase = ASSIO_SCORE_PHASE::HOLD;
		}
		return;
	}

	if (m_eAssioScorePhase == ASSIO_SCORE_PHASE::HOLD)
	{
		constexpr _float HOLD_DURATION = 1.f;
		if (m_fAssioScorePhaseElapsed >= HOLD_DURATION)
		{
			m_fAssioScorePhaseElapsed = 0.f;
			m_AssioCenterScoreMoveStart = center->GetPos();
			m_eAssioScorePhase = ASSIO_SCORE_PHASE::MOVE;
		}
		return;
	}

	constexpr _float MOVE_DURATION = 0.3f;
	const _float ratio = std::clamp(
		m_fAssioScorePhaseElapsed / MOVE_DURATION, 0.f, 1.f);
	center->SetPos({
		std::lerp(m_AssioCenterScoreMoveStart.x,
			m_AssioCenterScoreMoveTarget.x, ratio),
		std::lerp(m_AssioCenterScoreMoveStart.y,
			m_AssioCenterScoreMoveTarget.y, ratio)
	});
	center->SetScaleRatio(std::lerp(
		m_fAssioCenterScoreBaseScale,
		std::max(0.05f, targetScore->GetScaleRatio()),
		ratio));
	center->CalcUICoord();

	if (ratio < 1.f)
		return;

	m_iAssioPlayerScore = m_iAssioPendingPlayerScore;
	m_iAssioNpcScore = m_iAssioPendingNpcScore;
	if (auto* text = dynamic_cast<CTextBox*>(
		GetSafeUI(*m_hAssioPlayerScoreText)))
	{
		text->SetwText(std::to_wstring(m_iAssioPlayerScore));
	}
	if (auto* text = dynamic_cast<CTextBox*>(
		GetSafeUI(*m_hAssioNpcScoreText)))
	{
		text->SetwText(std::to_wstring(m_iAssioNpcScore));
	}

	center->SetAlpha(0.f);
	center->SetScaleRatio(0.f);
	center->SetPos(m_AssioCenterScoreBasePosition);
	center->CalcUICoord();
	m_iAssioPendingScore = 0;
	m_fAssioScorePhaseElapsed = 0.f;
	m_eAssioScorePhase = ASSIO_SCORE_PHASE::IMPACT_HOLD;
}

void UIManager::PlayAssioScoreImpactEffect()
{
	if (!m_hAssioTargetScoreText)
		return;

	auto* targetScore = GetSafeUI(*m_hAssioTargetScoreText);
	const auto& targetRoot = m_bAssioTargetIsPlayer ?
		m_hAssioPlayerScoreRoot : m_hAssioNpcScoreRoot;
	if (!targetScore || !targetRoot)
		return;
	auto* scoreRootUI = GetSafeUI(*targetRoot);
	if (!scoreRootUI)
		return;

	const std::vector<CHandle> scoreRoot{ *targetRoot };
	auto* auraTemplate = FindUIByNameRecursive(scoreRoot, "ScoreAura");
	if (!auraTemplate)
		return;

	// Keep the impact attached to the score row. The row starts moving as soon
	// as the turn changes, so a root-level effect would remain at the old spot.
	const _float parentScale = std::max(0.001f,
		scoreRootUI->GetScaleRatio());
	const _float2 rootPosition = scoreRootUI->GetPos();
	const _float2 effectLocalPosition{
		(m_AssioCenterScoreMoveTarget.x - rootPosition.x) / parentScale,
		(m_AssioCenterScoreMoveTarget.y - rootPosition.y) / parentScale
	};
	// The prefab aura is intentionally large. The impact begins at roughly half
	// that size so it hugs the score glyphs instead of covering the whole row.
	const _float ringWidth = auraTemplate->GetSize().x * 0.55f;
	const _float ringHeight = auraTemplate->GetSize().y * 0.55f;
	const uint32_t effectWeight = static_cast<uint32_t>(std::max(
		0, targetScore->GetWeight() + 2));

	CUIObject::UIOBJECT_DESC ringDesc{};
	ringDesc.sObjectTag = "AssioScoreImpactRing";
	ringDesc.Name = ringDesc.sObjectTag;
	ringDesc.fX = m_AssioCenterScoreMoveTarget.x;
	ringDesc.fY = m_AssioCenterScoreMoveTarget.y;
	ringDesc.fSizeX = std::max(1.f, ringWidth);
	ringDesc.fSizeY = std::max(1.f, ringHeight);
	ringDesc.fAlpha = 0.85f;
	ringDesc.ResTag = "TEX_UI_T_ScoreAuraRing";
	ringDesc.ResWeight = effectWeight + 1u;
	ringDesc.UIType = ETOUI(UI_TYPE::TEXUI);
	const auto ringHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		m_CurrentLevel,
		"Prototype_GameObject_TextureUI",
		"Layer_UI",
		&ringDesc);
	if (ringHandle)
	{
		auto* ring = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(*ringHandle);
		if (ring)
		{
			const CHandle safeHandle = *ringHandle;
			ring->SetParent(*targetRoot);
			scoreRootUI->AddChildren(safeHandle);
			auto& ringInfo = ring->GetUIInfo();
			ringInfo.LocalX = effectLocalPosition.x;
			ringInfo.LocalY = effectLocalPosition.y;
			ringInfo.WeightOffset = targetScore->GetUIInfo().WeightOffset + 3;
			ring->SetColor({ 0.35f, 0.95f, 0.65f });
			ring->SetLocalScaleRatio(1.f);
			ring->SetAlphaRatio(0.85f);
			ring->CalcUICoord();
			ring->Appear = [safeHandle](CUIObject*)
			{
				auto* current = GetSafeUI(safeHandle);
				if (!current)
					return;
				if (auto* tween = current->GetTweenCom())
				{
					tween->PlayTween(
						0.8f, 1.5f, 0.2f,
						[safeHandle](_float value)
						{
							if (auto* ui = GetSafeUI(safeHandle))
							{
								ui->SetLocalScaleRatio(value);
								ui->CalcUICoord();
							}
						}, nullptr, EEaseType::EaseOutQuad);
					tween->PlayTween(
						0.85f, 0.f, 0.2f,
						[safeHandle](_float value)
						{
							if (auto* ui = GetSafeUI(safeHandle))
								ui->SetAlphaRatio(value);
						}, [safeHandle]()
						{
							GET_SINGLE(UIManager)->DeleteUIRecursive(safeHandle);
							GET_SINGLE(UIManager)->CompleteAssioScoreImpact();
						}, EEaseType::EaseOutQuad);
				}
			};
		}
	}

	CEffectUI::FLIPBOOK_DESC smokeDesc{};
	smokeDesc.sObjectTag = "AssioScoreImpactSmoke";
	smokeDesc.Name = smokeDesc.sObjectTag;
	smokeDesc.fX = m_AssioCenterScoreMoveTarget.x;
	smokeDesc.fY = m_AssioCenterScoreMoveTarget.y;
	smokeDesc.fSizeX = std::max(1.f, ringWidth * 0.76f);
	smokeDesc.fSizeY = std::max(1.f, ringHeight * 0.76f);
	smokeDesc.fAlpha = 0.55f;
	smokeDesc.ResTag = "TEX_VFX_T_SmokeMedium_8x8_D";
	smokeDesc.ResWeight = effectWeight;
	smokeDesc.UIType = ETOUI(UI_TYPE::FLIPBOOK);
	smokeDesc.cellsize = 1024u;
	smokeDesc.Padding = 2u;
	smokeDesc.TotalFrame = 64u;
	smokeDesc.Columns = 8u;
	smokeDesc.Rows = 8u;
	smokeDesc.Duration = 0.65f;
	const auto smokeHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		m_CurrentLevel,
		"Prototype_GameObject_EffectUI",
		"Layer_UI",
		&smokeDesc);
	if (smokeHandle)
	{
		auto* smoke = E::CGameInstance::Get().
			GetGameObjectByHandleT<CEffectUI>(*smokeHandle);
		if (smoke)
		{
			const CHandle safeHandle = *smokeHandle;
			smoke->SetParent(*targetRoot);
			scoreRootUI->AddChildren(safeHandle);
			auto& smokeInfo = smoke->GetUIInfo();
			smokeInfo.LocalX = effectLocalPosition.x;
			smokeInfo.LocalY = effectLocalPosition.y;
			smokeInfo.WeightOffset = targetScore->GetUIInfo().WeightOffset + 2;
			smoke->SetColor({ 0.35f, 0.85f, 0.60f });
			smoke->SetAdditiveBlend(true);
			smoke->SetLocalScaleRatio(0.96f);
			smoke->SetAlphaRatio(0.52f);
			smoke->CalcUICoord();
			smoke->Appear = [safeHandle](CUIObject*)
			{
				auto* current = GetSafeUI(safeHandle);
				if (!current)
					return;
				if (auto* tween = current->GetTweenCom())
				{
					tween->PlayTween(
						0.8f, 1.5f, 0.2f,
						[safeHandle](_float value)
						{
							if (auto* ui = GetSafeUI(safeHandle))
							{
								ui->SetLocalScaleRatio(value);
								ui->CalcUICoord();
							}
						}, nullptr, EEaseType::EaseOutQuad);
					tween->PlayTween(
						0.52f, 0.f, 0.2f,
						[safeHandle](_float value)
						{
							if (auto* ui = GetSafeUI(safeHandle))
								ui->SetAlphaRatio(value);
						}, [safeHandle]()
						{
							GET_SINGLE(UIManager)->DeleteUIRecursive(safeHandle);
						}, EEaseType::EaseOutQuad);
				}
			};
		}
	}
}

void UIManager::CompleteAssioScoreImpact()
{
	if (!m_bAssioMiniGameActive ||
		m_eAssioScorePhase != ASSIO_SCORE_PHASE::IMPACT_HOLD)
	{
		return;
	}

	m_fAssioScorePhaseElapsed = 0.f;
	if (m_bAssioFinalScore)
		BeginAssioResult();
	else
		BeginAssioTurnChange();
}

void UIManager::BeginAssioTurnChange()
{
	m_fAssioScorePhaseElapsed = 0.f;
	m_bAssioTurnTitleFadeInStarted = false;
	if (m_hAssioTurnTitle)
	{
		if (auto* title = GetSafeUI(*m_hAssioTurnTitle))
			m_bAssioTurnTitleWasAlreadyHidden = title->GetAlpha() <= 0.001f;
		else
			m_bAssioTurnTitleWasAlreadyHidden = true;
	}
	else
	{
		m_bAssioTurnTitleWasAlreadyHidden = true;
	}
	m_eAssioScorePhase = ASSIO_SCORE_PHASE::TURN_CHANGE;
	if (!m_bAssioTurnTitleWasAlreadyHidden)
		TurnTitleFadeOut(0.2f);
}

void UIManager::BeginAssioResult()
{
	for (const CHandle root : m_AssioMiniGameRoots)
	{
		if (GetSafeUI(root))
			PlayFadeOutDelete(root, 0.f, 0.3f);
	}
	m_AssioMiniGameRoots.clear();
	ClearAssioGameplayHandles();
	m_fAssioScorePhaseElapsed = 0.f;
	m_eAssioScorePhase = ASSIO_SCORE_PHASE::RESULT_COAT_FADE_OUT;
}

void UIManager::LoadAssioResult()
{
	m_AssioResultRoots = LoadPrefab("AccioSuccess");
	if (m_AssioResultRoots.empty())
	{
		m_bAssioFinalScore = false;
		m_bAssioMiniGameActive = false;
		m_eAssioScorePhase = ASSIO_SCORE_PHASE::NONE;
		PlayFadeInAll2DUI(0.f, 0.5f);
		return;
	}
	PlayAssioUISound(ASSIO_UI_END_SOUND_PATH, 0.8f);

	// 동점일 때는 플레이어를 우선한다.
	const _bool playerWon = m_iAssioPlayerScore >= m_iAssioNpcScore;
	const wchar_t* winnerName = playerWon ? L"이솝 샤프" : L"저스티스 훈";
	const int winnerScore = playerWon ? m_iAssioPlayerScore : m_iAssioNpcScore;
	if (auto* winName = dynamic_cast<CTextBox*>(
		FindUIByNameRecursive(m_AssioResultRoots, "WinName")))
	{
		winName->SetwText(winnerName);
	}
	if (auto* score = dynamic_cast<CTextBox*>(
		FindUIByNameRecursive(m_AssioResultRoots, "Score")))
	{
		score->SetwText(std::to_wstring(winnerScore));
	}
	PlayRaceRootsFadeIn(m_AssioResultRoots, 0.35f);
	m_fAssioScorePhaseElapsed = 0.f;
	m_eAssioScorePhase = ASSIO_SCORE_PHASE::RESULT_HOLD;
}

void UIManager::ClearAssioGameplayHandles()
{
	m_hAssioScoreBoard.reset();
	m_hAssioPlayerScoreRoot.reset();
	m_hAssioNpcScoreRoot.reset();
	m_hAssioPlayerScoreText.reset();
	m_hAssioNpcScoreText.reset();
	m_hAssioPlayerFrame.reset();
	m_hAssioNpcFrame.reset();
	m_hAssioCenterScore.reset();
	m_hAssioCenterScoreText.reset();
	m_hAssioTurnTitle.reset();
	m_hAssioTargetScoreText.reset();
	m_iAssioPendingScore = 0;
	m_iAssioPendingPlayerScore = 0;
	m_iAssioPendingNpcScore = 0;
	m_bAssioTurnTitleFadeInStarted = false;
	m_bAssioTurnTitleWasAlreadyHidden = false;
}

void UIManager::ClearAssioMiniGameUI(_bool immediate)
{
	for (const CHandle root : m_AssioMiniGameRoots)
	{
		if (!GetSafeUI(root))
			continue;

		if (immediate)
			DeleteUIRecursive(root);
		else
			PlayFadeOutDelete(root, 0.f, 0.3f);
	}
	m_AssioMiniGameRoots.clear();
	for (const CHandle root : m_AssioResultRoots)
	{
		if (!GetSafeUI(root))
			continue;
		if (immediate)
			DeleteUIRecursive(root);
		else
			PlayFadeOutDelete(root, 0.f, 0.3f);
	}
	m_AssioResultRoots.clear();
	ClearAssioGameplayHandles();
	m_fAssioScorePhaseElapsed = 0.f;
	m_bAssioCurrentTurnIsPlayer = true;
	m_bAssioFinalScore = false;
	m_eAssioScorePhase = ASSIO_SCORE_PHASE::NONE;
	m_bAssioMiniGameActive = false;
}

void UIManager::StartRaceMiniGame()
{
	ClearRaceMiniGameUI();
	m_bRaceReturnPositionApplied = false;
	m_bRaceResultFadeOutStarted = false;
	m_fRaceReturnElapsed = 0.f;
	// Hide only the existing HUD before creating the race UI. The countdown,
	// RaceBoard and result Flag created below must remain visible.
	PlayFadeOutAll2DUI(0.f, 0.35f);
	FadeOutQuest(0.3f);
	m_fRaceMiniGameElapsed = 0.f;
	m_iRaceMiniGameCoinCount = 0u;
	m_eRaceMiniGamePhase = RACE_MINIGAME_PHASE::NONE;

	StartRaceStartTimer();
	if (IsRaceStartTimerPlaying())
		m_eRaceMiniGamePhase = RACE_MINIGAME_PHASE::COUNTDOWN;
	else
		FadeInQuest(0.5f);
}

void UIManager::AddRaceMiniGameCoin(uint32_t amount)
{
	if (m_eRaceMiniGamePhase != RACE_MINIGAME_PHASE::RACING)
		return;

	constexpr uint32_t MAX_RACE_COIN_COUNT = 99u;
	m_iRaceMiniGameCoinCount = std::min(
		MAX_RACE_COIN_COUNT,
		m_iRaceMiniGameCoinCount + amount);

	if (!m_hRaceBoardCoinText)
		return;
	if (auto* textBox = Engine::Cast<CTextBox>(
		GetSafeUI(*m_hRaceBoardCoinText)))
	{
		wchar_t coinText[16]{};
		swprintf_s(
			coinText,
			std::size(coinText),
			L"%02u / %u",
			m_iRaceMiniGameCoinCount,
			MAX_RACE_COIN_COUNT);
		textBox->SetwText(coinText);
	}
}

void UIManager::BeginRaceBoard()
{
	m_RaceBoardRoots = LoadPrefab("RaceBoard");
	m_hRaceBoardTimerText.reset();
	m_hRaceBoardCoinText.reset();
	m_fRaceMiniGameElapsed = 0.f;

	if (auto* timer = FindUIByNameRecursive(
		m_RaceBoardRoots, "TimerTitle"))
	{
		m_hRaceBoardTimerText = timer->GetHandle();
		if (auto* textBox = Engine::Cast<CTextBox>(timer))
			textBox->SetFixedDigitLayout(true);
	}
	// RaceBoard has two objects named Coin. The first is the numeric counter
	// and is intentionally found first by the hierarchy traversal.
	if (auto* coin = FindUIByNameRecursive(m_RaceBoardRoots, "Coin"))
	{
		m_hRaceBoardCoinText = coin->GetHandle();
		if (auto* textBox = Engine::Cast<CTextBox>(coin))
		{
			textBox->SetColoredSuffix(
				L"99",
				{ 0.39215687f, 1.f, 0.39215687f });
		}
	}

	if (!m_hRaceBoardTimerText || !m_hRaceBoardCoinText)
	{
		ClearRaceMiniGameUI();
		m_eRaceMiniGamePhase = RACE_MINIGAME_PHASE::NONE;
		FadeInQuest(0.5f);
		return;
	}

	m_eRaceMiniGamePhase = RACE_MINIGAME_PHASE::RACING;
	AddRaceMiniGameCoin(0u);
	PlayRaceRootsFadeIn(m_RaceBoardRoots);
}

void UIManager::FinishRaceMiniGame()
{
	if (m_eRaceMiniGamePhase != RACE_MINIGAME_PHASE::RACING)
		return;

	for (const CHandle root : m_RaceBoardRoots)
	{
		if (GetSafeUI(root))
			PlayFadeOutDelete(root, 0.f, 0.3f);
	}
	m_RaceBoardRoots.clear();
	m_hRaceBoardTimerText.reset();
	m_hRaceBoardCoinText.reset();

	m_RaceResultRoots = LoadPrefab("Flag");
	m_hRaceResultCoinText.reset();
	if (auto* coin = FindUIByNameRecursive(
		m_RaceResultRoots, "CoinCnt"))
	{
		m_hRaceResultCoinText = coin->GetHandle();
		if (auto* textBox = Engine::Cast<CTextBox>(coin))
		{
			wchar_t coinText[8]{};
			swprintf_s(
				coinText,
				std::size(coinText),
				L"%03u",
				m_iRaceMiniGameCoinCount);
			textBox->SetwText(coinText);
		}
	}

	m_eRaceMiniGamePhase = RACE_MINIGAME_PHASE::RESULT;
	PlayRaceRootsFadeIn(m_RaceResultRoots);
}

void UIManager::UpdateRaceMiniGame(_float fTimeDelta)
{
	constexpr _float RACE_DURATION = 120.f;
	constexpr _float RESULT_HOLD_DURATION = 5.f;
	constexpr _float RETURN_FADE_DURATION = 1.f;

	if (m_eRaceMiniGamePhase == RACE_MINIGAME_PHASE::COUNTDOWN)
	{
		if (!IsRaceStartTimerPlaying())
			BeginRaceBoard();
		return;
	}

	if (m_eRaceMiniGamePhase == RACE_MINIGAME_PHASE::RETURNING_TO_SHOP)
	{
		m_fRaceReturnElapsed += std::max(0.f, fTimeDelta);

		// BlackBG FadeIn starts after the result hold. Fade the Flag result UI
		// at exactly the same time so it disappears beneath the black screen.
		if (!m_bRaceResultFadeOutStarted &&
			m_fRaceReturnElapsed >= RESULT_HOLD_DURATION)
		{
			m_bRaceResultFadeOutStarted = true;
			for (const CHandle root : m_RaceResultRoots)
			{
				if (GetSafeUI(root))
					PlayFadeOutDelete(root, 0.f, RETURN_FADE_DURATION);
			}
			m_RaceResultRoots.clear();
			m_hRaceResultCoinText.reset();
		}

		if (m_fRaceReturnElapsed <
			RESULT_HOLD_DURATION + RETURN_FADE_DURATION)
			return;

		if (!m_bRaceReturnPositionApplied)
		{

			// 코인 게임 종료 후 복귀할 상점 좌표. 실제 상점 좌표로 교체한다.
			const _float3 vShopPosition{ 127.833f, 4.5f, -87.941f };
			const _float3 vShopLookAt{ 108.5f, 2.5f, -82.f };

			CPlayer* pPlayer = nullptr;
			if (const auto* pPlayerLayer = E::CGameInstance::Get().
				GetGameObjectLayer("03_Player"))
			{
				for (const CHandle hPlayer : *pPlayerLayer)
				{
					pPlayer = E::CGameInstance::Get().
						GetGameObjectByHandleT<CPlayer>(hPlayer);
					if (pPlayer)
						break;
				}
			}

			if (pPlayer)
			{
				pPlayer->SetFlyRequested(false);
				pPlayer->SetDialoguePose(vShopPosition, vShopLookAt);
			}
			if (m_OnRaceReturnToShop)
				m_OnRaceReturnToShop();

			m_bRaceReturnPositionApplied = true;
			CreateFadeOut(0.f, RETURN_FADE_DURATION);
			// Restore the original HUD while the black screen fades away.
			PlayFadeInAll2DUI(0.f, RETURN_FADE_DURATION);
			FadeInQuest(RETURN_FADE_DURATION);
		}

		m_eRaceMiniGamePhase = RACE_MINIGAME_PHASE::RESULT;
		return;
	}

	if (m_eRaceMiniGamePhase != RACE_MINIGAME_PHASE::RACING)
		return;

	m_fRaceMiniGameElapsed += std::max(0.f, fTimeDelta);
	const _float remaining = std::max(
		0.f,
		RACE_DURATION - m_fRaceMiniGameElapsed);

	if (m_hRaceBoardTimerText)
	{
		if (auto* textBox = Engine::Cast<CTextBox>(
			GetSafeUI(*m_hRaceBoardTimerText)))
		{
			const uint32_t totalCentiseconds = static_cast<uint32_t>(
				std::ceil(remaining * 100.f));
			const uint32_t minutes = totalCentiseconds / 6000u;
			const uint32_t seconds = (totalCentiseconds / 100u) % 60u;
			const uint32_t centiseconds = totalCentiseconds % 100u;
			wchar_t timerText[16]{};
			swprintf_s(
				timerText,
				std::size(timerText),
				L"%u:%02u:%02u",
				minutes,
				seconds,
				centiseconds);
			textBox->SetwText(timerText);
		}
	}

	if (m_fRaceMiniGameElapsed >= RACE_DURATION)
	{
		FinishRaceMiniGame();
		m_fRaceReturnElapsed = 0.f;
		m_bRaceReturnPositionApplied = false;
		m_bRaceResultFadeOutStarted = false;
		m_eRaceMiniGamePhase = RACE_MINIGAME_PHASE::RETURNING_TO_SHOP;
		CreateFadeIn(RESULT_HOLD_DURATION, RETURN_FADE_DURATION);
	}
}

void UIManager::ClearRaceMiniGameUI()
{
	for (const CHandle root : m_RaceBoardRoots)
	{
		if (GetSafeUI(root))
			PlayFadeOutDelete(root, 0.f, 0.25f);
	}
	for (const CHandle root : m_RaceResultRoots)
	{
		if (GetSafeUI(root))
			PlayFadeOutDelete(root, 0.f, 0.25f);
	}

	m_RaceBoardRoots.clear();
	m_RaceResultRoots.clear();
	m_hRaceBoardTimerText.reset();
	m_hRaceBoardCoinText.reset();
	m_hRaceResultCoinText.reset();
}

void UIManager::PlayRaceRootsFadeIn(
	const std::vector<CHandle>& roots,
	_float playtime)
{
	for (const CHandle root : roots)
	{
		auto* ui = GetSafeUI(root);
		if (!ui)
			continue;

		const _float targetAlpha = ui->GetAlpha();
		ui->SetAlpha(0.f);

		// UI의 첫 APPEAR 처리에서는 각 UI 클래스가 기존 Tween을 지운다.
		// 로드 직후 Tween을 시작하면 다음 프레임 APPEAR에서 삭제되므로,
		// APPEAR 콜백 안에서 FadeIn Tween을 등록해야 한다.
		ui->Appear = [root, targetAlpha, playtime](CUIObject*)
		{
			auto* target = GetSafeUI(root);
			if (!target)
				return;

			target->SetAlpha(0.f);
			if (auto* tween = target->GetTweenCom())
			{
				tween->PlayTween(
					0.f,
					targetAlpha,
					std::max(0.f, playtime),
					[root](_float value)
					{
						if (auto* current = GetSafeUI(root))
							current->SetAlpha(value);
					}, nullptr, EEaseType::EaseOutQuad);
			}
			else
			{
				target->SetAlpha(targetAlpha);
			}
		};
	}
}

void UIManager::StartRaceStartTimer()
{
	// 디버그 키를 연속으로 눌러도 이전 타이머가 중복되지 않도록 정리한다.
	for (const CHandle root : m_RaceStartTimerRoots)
		DeleteUIRecursive(root);

	m_RaceStartTimerRoots = LoadPrefab("RaceStartTimer");
	m_hRaceStartTimerFrame.reset();
	m_hRaceStartTimerNumberPad.reset();
	m_hRaceStartTimerText.reset();
	m_hRaceStartTimerFire.reset();
	m_hRaceStartTimerFlagR.reset();
	m_hRaceStartTimerFlagL.reset();

	if (auto* frame = FindUIByNameRecursive(m_RaceStartTimerRoots, "StartTimerBG"))
	{
		m_hRaceStartTimerFrame = frame->GetHandle();
		m_fRaceStartTimerFrameBaseScale = frame->GetScaleRatio();
		frame->SetAlpha(1.f);
	}

	if (auto* numberPad = FindUIByNameRecursive(m_RaceStartTimerRoots, "NumberPad"))
	{
		m_hRaceStartTimerNumberPad = numberPad->GetHandle();
		m_fRaceStartTimerNumberPadBaseScale = numberPad->GetScaleRatio();
		numberPad->SetAlpha(0.f);
		numberPad->SetScaleRatio(0.f);
	}

	if (auto* text = FindUIByNameRecursive(m_RaceStartTimerRoots, "64px"))
	{
		m_hRaceStartTimerText = text->GetHandle();
		m_RaceStartTimerTextBaseLocalPos = {
			text->GetUIInfo().LocalX,
			text->GetUIInfo().LocalY
		};
		text->SetColor({ 1.f, 1.f, 1.f });
		if (auto* textBox = Engine::Cast<CTextBox>(text))
			textBox->SetwText(L"3");
	}

	if (auto* fire = FindUIByNameRecursive(m_RaceStartTimerRoots, "Fire"))
	{
		m_hRaceStartTimerFire = fire->GetHandle();
		m_fRaceStartTimerFireBaseAlpha = fire->GetAlpha();
	}

	if (auto* flagR = FindUIByNameRecursive(m_RaceStartTimerRoots, "FlagR"))
	{
		m_hRaceStartTimerFlagR = flagR->GetHandle();
		m_fRaceStartTimerFlagRBaseAlpha = flagR->GetAlpha();
	}

	if (auto* flagL = FindUIByNameRecursive(m_RaceStartTimerRoots, "FlagL"))
	{
		m_hRaceStartTimerFlagL = flagL->GetHandle();
		m_fRaceStartTimerFlagLBaseAlpha = flagL->GetAlpha();
	}

	m_fRaceStartTimerElapsed = 0.f;
	m_bRaceStartTimerPlaying = m_hRaceStartTimerFrame.has_value() &&
		m_hRaceStartTimerNumberPad.has_value() &&
		m_hRaceStartTimerText.has_value() &&
		m_hRaceStartTimerFire.has_value() &&
		m_hRaceStartTimerFlagR.has_value() &&
		m_hRaceStartTimerFlagL.has_value();

	if (!m_bRaceStartTimerPlaying)
	{
		for (const CHandle root : m_RaceStartTimerRoots)
			DeleteUIRecursive(root);
		m_RaceStartTimerRoots.clear();
	}
}

void UIManager::UpdateRaceStartTimer(_float fTimeDelta)
{
	if (!m_bRaceStartTimerPlaying || !m_hRaceStartTimerFrame ||
		!m_hRaceStartTimerNumberPad || !m_hRaceStartTimerText ||
		!m_hRaceStartTimerFire || !m_hRaceStartTimerFlagR ||
		!m_hRaceStartTimerFlagL)
	{
		return;
	}

	auto* frame = GetSafeUI(*m_hRaceStartTimerFrame);
	auto* numberPad = GetSafeUI(*m_hRaceStartTimerNumberPad);
	auto* text = GetSafeUI(*m_hRaceStartTimerText);
	auto* fire = GetSafeUI(*m_hRaceStartTimerFire);
	auto* flagR = GetSafeUI(*m_hRaceStartTimerFlagR);
	auto* flagL = GetSafeUI(*m_hRaceStartTimerFlagL);
	if (!frame || !numberPad || !text || !fire || !flagR || !flagL)
	{
		m_bRaceStartTimerPlaying = false;
		m_RaceStartTimerRoots.clear();
		return;
	}

	constexpr _float countDuration = 1.f;
	constexpr _float goDuration = 1.f;
	constexpr _float countTotal = countDuration * 3.f;
	constexpr _float totalDuration = countTotal + goDuration;
	const auto saturate = [](_float value)
	{
		return std::clamp(value, 0.f, 1.f);
	};
	const auto easeOutQuad = [&saturate](_float value)
	{
		const _float t = saturate(value);
		return 1.f - (1.f - t) * (1.f - t);
	};
	const auto heartbeat = [&saturate](_float localTime)
	{
		// 매초 시작 시점부터 0.2초 동안 한 번 작아졌다가 원래 크기로 복귀한다.
		const _float secondPhase = std::fmod(std::max(localTime, 0.f), 1.f);
		if (secondPhase >= 0.2f)
			return 0.f;
		return std::sin(saturate(secondPhase / 0.2f) * XM_PI) * 0.09f;
	};

	m_fRaceStartTimerElapsed += fTimeDelta;
	const _float elapsed = m_fRaceStartTimerElapsed;
	if (elapsed >= totalDuration)
	{
		for (const CHandle root : m_RaceStartTimerRoots)
			DeleteUIRecursive(root);
		m_RaceStartTimerRoots.clear();
		m_hRaceStartTimerFrame.reset();
		m_hRaceStartTimerNumberPad.reset();
		m_hRaceStartTimerText.reset();
		m_hRaceStartTimerFire.reset();
		m_hRaceStartTimerFlagR.reset();
		m_hRaceStartTimerFlagL.reset();
		m_bRaceStartTimerPlaying = false;
		return;
	}

	if (elapsed < countTotal)
	{
		const uint32_t countIndex = std::min(
			2u, static_cast<uint32_t>(elapsed / countDuration));
		const _float localTime = elapsed - countDuration * countIndex;
		const wchar_t* countTexts[] = { L"3", L"2", L"1" };
		if (auto* textBox = Engine::Cast<CTextBox>(text))
			textBox->SetwText(countTexts[countIndex]);

		// The '1' glyph has asymmetric side bearings. Keep the text
		// mathematically centered, then apply a small optical correction only
		// for that glyph so 3/2/GO retain their authored position.
		constexpr _float ONE_OPTICAL_CENTER_OFFSET_X = -3.f;
		text->SetLocalPos({
			m_RaceStartTimerTextBaseLocalPos.x +
				(countIndex == 2u ? ONE_OPTICAL_CENTER_OFFSET_X : 0.f),
			m_RaceStartTimerTextBaseLocalPos.y
		});

		text->SetColor({ 1.f, 1.f, 1.f });
		_float textAlpha = saturate(localTime / 0.25f);
		if (localTime > 0.62f)
			textAlpha *= 1.f - saturate((localTime - 0.62f) / 0.34f);
		// NumberPad 알파 0.1 * 숫자 AlphaRatio 10 = 최종 숫자 알파 1.0.
		numberPad->SetAlpha(textAlpha * 0.1f);
		const _float numberSizeRatio = easeOutQuad(localTime / 0.32f);
		numberPad->SetScaleRatio(
			m_fRaceStartTimerNumberPadBaseScale * numberSizeRatio);

		// Fire 플립북은 건드리지 않고 원형 프레임만 매초 한 번 두근거린다.
		frame->SetScaleRatio(m_fRaceStartTimerFrameBaseScale *
			(1.f - heartbeat(elapsed)));
		const _float introAlpha = saturate(elapsed / 0.3f);
		frame->SetAlpha(introAlpha);
		fire->SetAlpha(m_fRaceStartTimerFireBaseAlpha * introAlpha);
		flagR->SetAlpha(m_fRaceStartTimerFlagRBaseAlpha * introAlpha);
		flagL->SetAlpha(m_fRaceStartTimerFlagLBaseAlpha * introAlpha);
		return;
	}

	const _float goTime = elapsed - countTotal;
	if (auto* textBox = Engine::Cast<CTextBox>(text))
		textBox->SetwText(L"GO");
	text->SetLocalPos(m_RaceStartTimerTextBaseLocalPos);
	text->SetColor({ 0.32f, 1.f, 0.28f });

	_float goAlpha = saturate(goTime / 0.25f);
	if (goTime > 0.62f)
		goAlpha *= 1.f - saturate((goTime - 0.62f) / 0.34f);
	numberPad->SetAlpha(goAlpha * 0.1f);
	const _float goSizeRatio = easeOutQuad(goTime / 0.32f);
	numberPad->SetScaleRatio(
		m_fRaceStartTimerNumberPadBaseScale * goSizeRatio);

	// GO 숫자만 남기고 원형 프레임, Fire, 양쪽 깃발은 함께 사라진다.
	const _float backgroundAlpha = 1.f - saturate(goTime / 0.3f);
	frame->SetAlpha(backgroundAlpha);
	fire->SetAlpha(m_fRaceStartTimerFireBaseAlpha * backgroundAlpha);
	flagR->SetAlpha(m_fRaceStartTimerFlagRBaseAlpha * backgroundAlpha);
	flagL->SetAlpha(m_fRaceStartTimerFlagLBaseAlpha * backgroundAlpha);
	frame->SetScaleRatio(m_fRaceStartTimerFrameBaseScale *
		(1.f - heartbeat(elapsed)));
}

void UIManager::UpdateWandShopWorldMousePosition()
{
	m_bWandShopPanelMouseHit = false;
	m_WandShopPanelMousePosition = { -FLT_MAX, -FLT_MAX };
	if (!m_bWandShopWorldMode)
		return;

	// Picking must use the same active camera that projects and billboards the
	// world RTT panel, including cinematic cameras.
	auto* camera = E::CGameInstance::Get().GetActiveCamera();
	if (!camera)
		return;

	const _float2 screenSize = E::CGameInstance::Get().GetClientScreenSize();
	const auto [rayOriginValue, rayDirectionValue] = camera->GetRayFromScreenPixel(
		E::CGameInstance::Get().GetMousePos(), screenSize);
	const _vector rayOrigin = XMLoadFloat3(&rayOriginValue);
	const _vector rayDirection = XMVector3Normalize(XMLoadFloat3(&rayDirectionValue));
	const _matrix panelWorld = XMLoadFloat4x4(&m_WandShopPanelWorld);
	const _vector panelPosition = panelWorld.r[3];
	const _vector panelNormal = XMVector3Normalize(panelWorld.r[2]);
	const _float denominator = XMVectorGetX(XMVector3Dot(rayDirection, panelNormal));
	if (std::abs(denominator) <= 0.00001f)
		return;

	const _float distance = XMVectorGetX(XMVector3Dot(
		panelPosition - rayOrigin, panelNormal)) / denominator;
	if (distance < 0.f)
		return;

	const _vector hitPosition = rayOrigin + rayDirection * distance;
	const _matrix inversePanel = XMMatrixInverse(nullptr, panelWorld);
	const _vector localHit = XMVector3TransformCoord(hitPosition, inversePanel);
	const _float localX = XMVectorGetX(localHit);
	const _float localY = XMVectorGetY(localHit);
	if (localX < -0.5f || localX > 0.5f ||
		localY < -0.5f || localY > 0.5f)
	{
		return;
	}

	const _float u = localX + 0.5f;
	const _float v = 0.5f - localY;
	m_WandShopPanelMousePosition = { u * screenSize.x, v * screenSize.y };
	m_bWandShopPanelMouseHit = true;
}

_float2 UIManager::GetUIInteractionMousePosition() const
{
	if (!m_bWandShopWorldMode)
		return E::CGameInstance::Get().GetMousePos();
	return m_bWandShopPanelMouseHit ?
		m_WandShopPanelMousePosition : _float2{ -FLT_MAX, -FLT_MAX };
}

void UIManager::Initialize(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
	m_pDevice = pDevice;
	m_pContext = pContext;

	MFStartup(MF_VERSION);
}

void UIManager::InitializeActions()
{
	m_EventMap["ClearAction"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pTween->ClearTweens();
		};
	m_vEventNames.push_back("ClearAction");

	// ==========================================
	// 1. 사이즈 업
	// ==========================================
	m_EventMap["ScaleUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float originScaleRatio = pCaller->GetScaleRatio();
		pTween->PlayTween(pCaller->GetScaleRatio(), 1.1f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}});
	};
	m_vEventNames.push_back("ScaleUp");

	m_EventMap["ScaleUp0.6"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			CHandle handle = pCaller->GetHandle();
			_float originScaleRatio = pCaller->GetScaleRatio();
			pTween->PlayTween(pCaller->GetScaleRatio(), 0.65f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}});
		};
	m_vEventNames.push_back("ScaleUp0.6");

	m_EventMap["AppearScaleUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		//pCaller->SetInputLcok(true);

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		_float scaleRatio = pCaller->GetScaleRatio();

		pTween->PlayTween(0.5f, scaleRatio, 0.2f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);

		pTween->PlayTween(0.f, 1.f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					pObj->SetAlpha(currentValue);
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);
	};
	m_vEventNames.push_back("AppearScaleUp");

	m_EventMap["AppearScaleUp0.1"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			CHandle handle = pCaller->GetHandle();
			_float scaleRatio = pCaller->GetScaleRatio();

			pTween->PlayTween(0.5f, scaleRatio, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 0.1f);

			pTween->PlayTween(0.f, 1.f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetAlpha(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 0.1f);
		};
	m_vEventNames.push_back("AppearScaleUp0.1");

	m_EventMap["AppearScaleUp1"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			CHandle handle = pCaller->GetHandle();
			_float scaleRatio = pCaller->GetScaleRatio();

			pTween->PlayTween(0.5f, scaleRatio, 0.2f,
				[handle, bSoundPlayed = false](float currentValue) mutable {
					if (auto pObj = GetSafeUI(handle))
					{
						//if (!bSoundPlayed)
						//{
						//	E::CGameInstance::Get()
						//		.GetSoundManager()
						//		->Play2D(
						//			"./Resources/SampleClient/Sound/UI/Paper.wav",
						//			SOUND_PLAY_DESC{
						//				.sBusID = SOUND_BUS::UI,
						//				.fVolume = 0.3f,
						//				.fPitch = 1.f,
						//				.iPriority = 64,
						//				.bLoop = false
						//			});
						//
						//	bSoundPlayed = true;
						//}

						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 1.f);

			pTween->PlayTween(0.f, 1.f, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetAlpha(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 1.f);
		};
	m_vEventNames.push_back("AppearScaleUp1");

	m_EventMap["AppearScaleUp1.1"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			CHandle handle = pCaller->GetHandle();
			_float scaleRatio = pCaller->GetScaleRatio();


			pTween->PlayTween(0.5f, scaleRatio, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 1.2f);

			pTween->PlayTween(0.f, 1.f, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetAlpha(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 1.2f);
		};
	m_vEventNames.push_back("AppearScaleUp1.1");

	m_EventMap["TextScaleUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		_float scaleRatio = pCaller->GetScaleRatio();
		_float2 originSize = pCaller->GetSize();

		pTween->PlayTween(0.5f, 1.f, 0.2f,
			[handle, originSize](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					pObj->SetSize({ originSize.x * currentValue,  originSize.y * currentValue });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);

		pTween->PlayTween(0.f, 1.f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					pObj->SetAlpha(currentValue);
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);
	};
	m_vEventNames.push_back("TextScaleUp");

	// ==========================================
	// 2. 사이즈 축소
	// ==========================================
	m_EventMap["ScaleDown"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 1.0f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			});
	};
	m_vEventNames.push_back("ScaleDown");

	m_EventMap["ScaleDown0.6"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			CHandle handle = pCaller->GetHandle();

			pTween->PlayTween(pCaller->GetScaleRatio(), 0.6f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}
				});
		};
	m_vEventNames.push_back("ScaleDown0.6");

	m_EventMap["DisappearScaleDown"] = [this](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetInputLcok(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 0.5f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			}, [handle, this]() {
				if (auto pObj = GetSafeUI(handle)) DeleteUIRecursive(handle);
				}, EEaseType::EaseOutQuad);
	};
	m_vEventNames.push_back("DisappearScaleDown");

	m_EventMap["DisappearScaleDown_D"] = [this](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetInputLcok(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 0.5f, 0.2f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);

		pTween->PlayTween(1.f, 0.f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					if (currentValue <= 1.f)
					{
						pObj->SetAlpha(currentValue);
						pObj->CalcUICoord();
					}
				}
			}, [handle, this]() {
				if (auto pObj = GetSafeUI(handle)) DeleteUIRecursive(handle);
			}, EEaseType::EaseOutQuad, 0.1f);
	};
	m_vEventNames.push_back("DisappearScaleDown_D");

	// ==========================================
	// 3. 페이드 인
	// ==========================================
	m_EventMap["FadeIn"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		_float originAlpha = pCaller->GetAlpha();
		pTween->PlayTween(0.f, originAlpha, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});
	};
	m_vEventNames.push_back("FadeIn");

	m_EventMap["LocalFadeIn"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		pTween->PlayTween(pCaller->GetAlphaRatio(), 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			});
	};
	m_vEventNames.push_back("LocalFadeIn");

	m_EventMap["LocalFadeIn0.2"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			CHandle handle = pCaller->GetHandle();
			pTween->PlayTween(pCaller->GetAlphaRatio(), 1.0f, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
				});
		};
	m_vEventNames.push_back("LocalFadeIn0.2");

	// ==========================================
	// 4. 페이드 아웃
	// ==========================================
	m_EventMap["FadeOut"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetAlpha(), 0.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			}, 
			[handle]() {
				if (auto pObj = GetSafeUI(handle)) pObj->SetActive(false);
			});
	};
	m_vEventNames.push_back("FadeOut");

	m_EventMap["LocalFadeOut"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		//pCaller->SetInputLcok(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetAlphaRatio(), 0.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			},
			[handle]() {
				//if (auto pObj = GetSafeUI(handle)) pObj->SetActive(false);
			});
	};
	m_vEventNames.push_back("LocalFadeOut");

	m_EventMap["LocalFadeOut0.2"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			CHandle handle = pCaller->GetHandle();

			pTween->PlayTween(pCaller->GetAlphaRatio(), 0.0f, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
				},
				[handle]() {
					//if (auto pObj = GetSafeUI(handle)) pObj->SetActive(false);
				});
		};
	m_vEventNames.push_back("LocalFadeOut0.2");

	m_EventMap["FadeOut_D"] = [this](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetInputLcok(true);
		pCaller->SetInputLcok(true);

		CHandle handle = pCaller->GetHandle();
		pTween->PlayTween(pCaller->GetAlpha(), 0.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			},
			[handle, this]() {
				if (auto pObj = GetSafeUI(handle)) DeleteUIRecursive(handle);
			});
	};
	m_vEventNames.push_back("FadeOut_D");

	m_EventMap["SpellEffect"] = [this](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			CHandle handle = pCaller->GetHandle();

			pTween->PlayTween(1.f, 1.5f, 1.f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetScaleRatio(currentValue);
				}, nullptr, EEaseType::EaseOutQuad);

			pTween->PlayTween(1.f, 0.0f, 1.f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
				},
				[handle, this]() {
					if (auto pObj = GetSafeUI(handle)) DeleteUIRecursive(handle);
				}, EEaseType::EaseOutQuad);
		};
	m_vEventNames.push_back("SpellEffect");

	// ==========================================
	// 5. 페이드 인 & 아웃
	// ==========================================
	m_EventMap["FadInOut"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(0.f, 1.f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			},
			[handle]() {
				if (auto pObj = GetSafeUI(handle))
				{
					if (auto pNextTween = pObj->GetTweenCom())
					{
						pNextTween->PlayTween(1.f, 0.f, 0.3f,
							[handle](float currentValue) {
								if (auto pObj2 = GetSafeUI(handle)) pObj2->SetAlphaRatio(currentValue);
							},
							[handle]() {
								if (auto pObj2 = GetSafeUI(handle)) pObj2->SetActive(false);
							});
					}
				}
			});
	};
	m_vEventNames.push_back("FadInOut");

	// ==========================================
	// 6. 스케일 업 & 다운
	// ==========================================
	m_EventMap["ScaleUpDown"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 1.2f, 0.08f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			},
			[handle]() {
				if (auto pObj = GetSafeUI(handle)) 
				{
					if (auto pNextTween = pObj->GetTweenCom()) 
					{
						pNextTween->PlayTween(1.2f, 1.1f, 0.08f,
							[handle](float currentValue) {
								if (auto pObj2 = GetSafeUI(handle)) {
									pObj2->SetScaleRatio(currentValue);
									pObj2->CalcUICoord();
								}
							});
					}
				}
			});
	};
	m_vEventNames.push_back("ScaleUpDown");

	// ==========================================
	// 위치 업
	// ==========================================
	m_EventMap["PosUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		pCaller->SetInputLcok(true);

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 30.f, 0.4f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ pObj->GetPos().x, originalPos.y - currentValue });
					pObj->CalcUICoord();
				}
			}, [handle]() {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetInputLcok(false);
				}
			});

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("PosUp");

	m_EventMap["LocalPosUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetLocalPos();
		pTween->PlayTween(0, 30.f, 0.4f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ pObj->GetLocalPos().x, originalPos.y - currentValue });
					pObj->CalcUICoord();
				}
			});

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			});
	};
	m_vEventNames.push_back("LocalPosUp");

	// ==========================================
	// 오른쪽
	// ==========================================
	m_EventMap["PosRight"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 30.f, 0.4f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ originalPos.x + currentValue, originalPos.y });
					pObj->CalcUICoord();
				}
			});

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("PosRight");

	// ==========================================
	// 바운스
	// ==========================================
	m_EventMap["Bounce"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		pCaller->SetActive(true);
		pCaller->SetInputLcok(true);

		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 150.f, 1.f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ originalPos.x, originalPos.y + currentValue });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutBounce);

		pTween->PlayTween(0, 80.f, 1.f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					_float2 pos = pObj->GetPos();
					pObj->SetPos({ originalPos.x + currentValue, pos.y });
					pObj->CalcUICoord();
				}
			}, [handle]() {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetInputLcok(false);
				}
			}, EEaseType::EaseOutQuad);

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("Bounce");


	// ==========================================
	// 탄성
	// ==========================================
	m_EventMap["Elastic"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 100.f, 1.f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ pObj->GetPos().x, originalPos.y + currentValue - 100.f });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutElastic);

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("Elastic");

	// ==========================================
	// 오버슛
	// ==========================================
	m_EventMap["OverShoot"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 50.f, 0.5f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ originalPos.x + currentValue, originalPos.y });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutBack);

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("OverShoot");

	// ==========================================
	// 둥둥
	// ==========================================
	m_EventMap["Floating"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		float startY = pCaller->GetUIInfo().fY;
		float endY = startY + 15.0f;

		pTween->PlayTween(startY, endY, 1.5f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->GetUIInfo().fY = currentValue;
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::Floating, 0.0f, true);
	};
	m_vEventNames.push_back("Floating");

	// ==========================================
	// 순차
	// ==========================================
	int maxIterations = 10;
	for (int i = 1; i <= maxIterations; ++i)
	{
		float delay = i * 0.3f;

		char szName[32];
		snprintf(szName, sizeof(szName), "PosUp%.1f", delay);
		std::string eventName = szName;

		m_EventMap[eventName] = [delay](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float2 originalPos = pCaller->GetPos();

			pTween->PlayTween(0.f, 30.f, 0.4f,
				[handle, originalPos](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetPos({ pObj->GetPos().x, originalPos.y - currentValue });
						pObj->CalcUICoord();
					}
				}, [handle]() {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetInputLcok(false);
					}
				}, EEaseType::Linear, delay, false);

			pTween->PlayTween(0.f, 1.0f, 0.3f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
				}, nullptr, EEaseType::Linear, delay, false);
		};
		m_vEventNames.push_back(eventName);
	}

	// ==========================================
	// 펄스
	// ==========================================
	m_EventMap["LockOnEffect"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		float startSizeX = pCaller->GetUIInfo().SizeX;
		float targetSizeX = startSizeX * 2.0f;

		pTween->PlayTween(startSizeX, targetSizeX, 0.6f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->GetUIInfo().SizeX = currentValue;
					pObj->GetUIInfo().SizeY = currentValue; 
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad, 0.0f, true); 

		pTween->PlayTween(1.0f, 0.0f, 0.6f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetAlphaRatio(currentValue);
				}
			}, nullptr, EEaseType::EaseOutQuad, 0.0f, true);
	};
	m_vEventNames.push_back("LockOnEffect");

	/****************텍스트 버튼용*******************/
	m_EventMap["TxtButtonScaleUp"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float originScaleRatio = pCaller->GetScaleRatio();
			pTween->PlayTween(originScaleRatio, 1.2f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}}, [handle]() {
						if (auto pObj = GetSafeUI(handle)) {
							pObj->SetInputLcok(false);
						}
						});
		};
	m_vEventNames.push_back("TxtButtonScaleUp");

	m_EventMap["TxtButtonScaleDown"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float originScaleRatio = pCaller->GetScaleRatio();
			pTween->PlayTween(originScaleRatio, 1.f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}}, [handle]() {
						if (auto pObj = GetSafeUI(handle)) {
							pObj->SetInputLcok(false);
						}
						});
		};
	m_vEventNames.push_back("TxtButtonScaleDown");

	m_EventMap["TxtButtonColorUp"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float3 originColor = pCaller->GetUIInfo().Color;
			pTween->PlayTween(1.f, 2.f, 0.1f,
				[handle, originColor](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->GetUIInfo().Color = { originColor.x * currentValue, originColor.y * currentValue, originColor.z * currentValue };
						//pObj->CalcUICoord();
					}}, [handle]() {
						if (auto pObj = GetSafeUI(handle)) {
							pObj->SetInputLcok(false);
						}
						});
		};
	m_vEventNames.push_back("TxtButtonColorUp");

	m_EventMap["TxtButtonColorDown"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float3 originColor = pCaller->GetUIInfo().Color;
			pTween->PlayTween(1.f, 0.5f, 0.1f,
				[handle, originColor](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->GetUIInfo().Color = { originColor.x * currentValue, originColor.y * currentValue, originColor.z * currentValue };
						//pObj->CalcUICoord();
					}
				}, [handle]() {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetInputLcok(false);
					}
				});
		};
	m_vEventNames.push_back("TxtButtonColorDown");
}

void UIManager::InitializeFunc()
{
	m_FuncMap["Create"] = [](std::string name)
	{
		GET_SINGLE(UIManager)->LoadPrefab(name);
	};
	m_vFuncNames.push_back("Create");

	m_FuncMap["SceneChange"] = [this](std::string name)
	{
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LOGO));
	};
	m_vFuncNames.push_back("SceneChange");

	m_FuncMap["SpellTypeDesCreate"] = [](std::string name)
		{
			GET_SINGLE(UIManager)->LoadPrefab(name);
		};
	m_vFuncNames.push_back("SpellTypeDesCreate");

	m_FuncMap["ClearDeathScene"] = [](std::string name)
		{
			if(std::nullopt != GET_SINGLE(UIManager)->GetUIController())
				E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*GET_SINGLE(UIManager)->GetUIController())->ClearDeathScene();
		};
	m_vFuncNames.push_back("ClearDeathScene");

	m_FuncMap["CreateSpellDragIcon"] = [this](std::string name)
		{
			std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

			CTextureUI::UIOBJECT_DESC Desc{};
			Desc.sObjectTag = "Select_Image";

			Desc.fSizeX = 150.f;
			Desc.fSizeY = 150.f;

			Desc.fX = g_iWinSizeX * 0.5f;
			Desc.fY = g_iWinSizeY * 0.5f;
			Desc.fAlpha = 1.f;
			Desc.UIType = ETOUI(UI_TYPE::SHORTCUT_ICON);
			Desc.ResWeight = 350;
			Desc.ResTag = name;

			std::optional<CHandle> m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer(currentLevel, "Prototype_GameObject_TextureUI", "Layer_UI_Texture", &Desc);
			CTextureUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*m_oSelectHandle);
			selectUI->SetMouseTracking(true);
			selectUI->SetUIType(ETOUI(UI_TYPE::SHORTCUT_ICON));


			if (m_UIController != std::nullopt &&
				nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*m_UIController))
			{
				CUIController* pController = E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*m_UIController);
				
				if (name == "TEX_UI_T_spellmeter_ArrestoMomentum_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_ARRESTOMOMENTUM));
				}
				else if (name == "TEX_UI_T_spellmeter_Glacius_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_GLACIUS));
				}
				else if (name == "TEX_UI_T_spellmeter_Levioso_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_LEVIOSO));
				}
				else if (name == "TEX_UI_T_spellmeter_TransformationOverlandOverlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_TRANSFORMATION));
				}
				else if (name == "TEX_UI_T_spellmeter_Accio_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_ASSIO));
				}
				else if (name == "TEX_UI_T_spellmeter_Depulso_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_DEPULSO));
				}
				else if (name == "TEX_UI_T_spellmeter_Descendo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_DESENDO));
				}
				else if (name == "TEX_UI_T_spellmeter_Flipendo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_FLIPENDO));
				}
				else if (name == "TEX_UI_T_spellmeter_Confringo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_CONFRINGO));
				}
				else if (name == "TEX_UI_T_spellmeter_Diffindo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_DIFFINDO));
				}
				else if (name == "TEX_UI_T_spellmeter_Expelliarmus_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_EXPELLIARMUS));
				}
				else if (name == "TEX_UI_T_spellmeter_Bombarda_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_BOMBARDA));
				}
				else if (name == "TEX_UI_T_spellmeter_Incendio_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_INCENDIO));
				}
				else if (name == "TEX_UI_T_spellmeter_Disillusionment_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_DISILLUSIONMENT));
				}
				else if (name == "TEX_UI_T_spellmeter_Lumos_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_LUMOS));
				}
				else if (name == "TEX_UI_T_spellmeter_Reparo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_REPARO));
				}
				else if (name == "TEX_UI_T_spellmeter_WingardiumLeviosa_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_WINGARDIUM));
				}
				else if (name == "TEX_UI_T_spellmeter_AvadaKedavra_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_AVADAKEDAVRA));
				}
				else if (name == "TEX_UI_T_spellmeter_Crucio_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_CRUCIO));
				}
				else if (name == "TEX_UI_T_spellmeter_Imperio_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_IMPERIO));
				}
			}
		};
	m_vFuncNames.push_back("CreateSpellDragIcon");
}

void UIManager::UpdateRootUIHandles()
{
	std::vector<Engine::CUIObject*> uiList;

	if (nullptr == CGameInstance::Get().GetGameObjectLayer("Layer_UI"))
		return;

	rootUIHandles.clear();

	const std::vector<CHandle>* uiHandles = CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	for (auto ui : *uiHandles)
	{
		Engine::CUIObject* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);

		if (checkUI != nullptr)
		{
			if (std::nullopt == checkUI->GetParent())
			{
				rootUIHandles.push_back(ui);
			}
		}
	}
}

std::function<void(CUIObject* pCaller)> UIManager::GetAction(const std::string& actionName)
{
	auto iter = m_EventMap.find(actionName);
	if (iter != m_EventMap.end())
		return iter->second;

	MSG_BOX("[UI Error] Action not found: ");
	return [](CUIObject*) {};
}

std::function<void(std::string text)> UIManager::GetFunc(const std::string& funcName)
{
	auto iter = m_FuncMap.find(funcName);
	if (iter != m_FuncMap.end())
		return iter->second;

	MSG_BOX("[UI Error] Func not found: ");
	return [](std::string text) {};
}

void UIManager::CreateFadeIn(float delay, float playtime)
{
	CHandle hBG{};
	if (m_hScreenFade && GetSafeUI(*m_hScreenFade))
	{
		hBG = *m_hScreenFade;
	}
	else
	{
		auto roots = LoadPrefab("BlackBG");
		if (roots.empty())
			return;

		hBG = roots.front();
		m_hScreenFade = hBG;
		if (auto* pBG = GetSafeUI(hBG))
			pBG->SetAlpha(0.f);
	}

	if (auto* pBG = GetSafeUI(hBG))
		pBG->GetTweenCom()->ClearTweens();
	PlayOnlyFadeIn(hBG, delay, playtime);
}

void UIManager::CreateFadeOut(float delay, float playtime,
	std::function<void()> onComplete)
{
	CHandle hBG{};
	if (m_hScreenFade && GetSafeUI(*m_hScreenFade))
	{
		hBG = *m_hScreenFade;
	}
	else
	{
		auto roots = LoadPrefab("BlackBG");
		if (roots.empty())
			return;

		hBG = roots.front();
		m_hScreenFade = hBG;
	}

	if (auto* pBG = GetSafeUI(hBG))
		pBG->GetTweenCom()->ClearTweens();
	PlayFadeOutDelete(hBG, delay, playtime, std::move(onComplete));
}

void UIManager::CreateFadeInSceneChange(float delay, float playtime, LEVEL level)
{
	CHandle hBG = GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front();
	PlayFadeInChange(hBG, level, delay, playtime);
}

void UIManager::CreateDamageFont(uint32_t damage, CHandle targetMonster, _bool isCritical)
{
	auto* pMonster =
		E::CGameInstance::Get()
		.GetGameObjectByHandleT<CMonster>(targetMonster);

	auto* pCamera =
		E::CGameInstance::Get().GetActiveCamera();

	if (!pMonster || !pCamera || damage == 0)
		return;

	const _float3 worldPosition =
		pMonster->GetHurtBoxPosition();

	const _float2 screenSize =
		E::CGameInstance::Get().GetClientScreenSize();

	const _vector world =
		XMLoadFloat3(&worldPosition);

	const _matrix view = pCamera->GetView();
	const _matrix proj = pCamera->GetProj();

	// 카메라 뒤쪽 검사
	const _vector clipPosition =
		XMVector4Transform(
			XMVectorSet(
				worldPosition.x,
				worldPosition.y,
				worldPosition.z,
				1.f),
			view * proj);

	if (XMVectorGetW(clipPosition) <= 0.f)
		return;

	const _vector projected =
		XMVector3Project(
			world,
			0.f,
			0.f,
			screenSize.x,
			screenSize.y,
			0.f,
			1.f,
			proj,
			view,
			XMMatrixIdentity());

	_float3 screenPosition{};
	XMStoreFloat3(&screenPosition, projected);

	if (screenPosition.z < 0.f || screenPosition.z > 1.f)
		return;

	// 랜덤 오프셋
	static std::mt19937 generator{ std::random_device{}() };
	static std::uniform_real_distribution<float> offsetX{ -25.f, 25.f };
	static std::uniform_real_distribution<float> offsetY{ -35.f, -10.f };

	//auto handles = LoadPrefab("DamageFont");
	//if (handles.empty())
	//	return;



	//const CHandle hDamageFont = handles.front();

	m_CurrentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	CTextUI::TEXT_DESC desc{};

	desc.sObjectTag = "DamageFont";
	desc.Name = "DamageFont";
	desc.fSizeX = 1.1f;
	desc.fSizeY = 1.1f;
	desc.fAlpha = 1.f;
	desc.Text = L"";
	desc.FontType = TEXT_FONT_TYPE::HAKGYOANSIM_PUZZLE_OUTLINE_25;
	desc.ResWeight = 1;

	std::optional<CHandle> hDamageFont = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextBox", "Layer_UI", &desc);
	auto* pDamageFont = E::CGameInstance::Get()
		.GetGameObjectByHandleT<CTextBox>(*hDamageFont);

	if (!pDamageFont)
		return;

	pDamageFont->SetwText(std::to_wstring(damage));
	pDamageFont->SetPos({
		screenPosition.x + offsetX(generator),
		screenPosition.y + offsetY(generator)
		});

	if (isCritical)
	{
		pDamageFont->SetColor({ 0.72f, 0.64f, 0.40f });
		pDamageFont->SetSize({ 0.9f, 0.9f });
		pDamageFont->SetScaleRatio(1.25f);
	}
	else
	{
		pDamageFont->SetColor({ 1.f, 1.f, 1.f });
		pDamageFont->SetSize({ 0.72f, 0.72f });
		pDamageFont->SetScaleRatio(1.f);
	}

	pDamageFont->SetAlpha(0.f);
	pDamageFont->CalcUICoord();

	PlayFadeIn(*hDamageFont, 0.f, 0.12f);
	PlayPosUP(*hDamageFont, 0.12f, 0.7f);
	PlayFadeOutDelete(*hDamageFont, 0.3f, 0.65f);
}

void UIManager::CreateActiveButton(CHandle handle, _ubyte KeyType)
{
	auto* pTarget = E::CGameInstance::Get().GetGameObjectByHandle(handle);
	if (!pTarget || pTarget->GetPendingDestroy())
		return;

	const auto duplicate = std::find_if(
		m_ActiveButtons.begin(),
		m_ActiveButtons.end(),
		[handle](const ACTIVE_BUTTON_INFO& info)
		{
			return info.TargetHandle == handle && !info.Removing;
		});

	if (duplicate != m_ActiveButtons.end())
		return;

	std::string resourceTag;
	_ubyte inputKey{};

	if (KeyType == static_cast<_ubyte>(ACTIVE_BUTTON_KEY::E) || KeyType == DIK_E)
	{
		resourceTag = "TEX_UI_T_cbi_Keyboard_E";
		inputKey = DIK_E;
	}
	else if (KeyType == static_cast<_ubyte>(ACTIVE_BUTTON_KEY::F) || KeyType == DIK_F)
	{
		resourceTag = "TEX_UI_T_cbi_Keyboard_F";
		inputKey = DIK_F;
	}
	else if (KeyType == static_cast<_ubyte>(ACTIVE_BUTTON_KEY::X) || KeyType == DIK_X)
	{
		resourceTag = "TEX_UI_T_cbi_Keyboard_X";
		inputKey = DIK_X;
	}
	else
	{
		return;
	}

	const std::string currentLevel =
		_string("LEVEL_") +
		MagicEnumToStringView(
			static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	CTextureUI::UIOBJECT_DESC desc{};
	desc.sObjectTag = "ActiveButton";
	desc.Name = "ActiveButton";
	desc.fSizeX = 32.f;
	desc.fSizeY = 32.f;
	desc.fAlpha = 0.f;
	desc.ResWeight = 10;
	desc.ResTag = resourceTag;
	desc.UIType = ETOUI(UI_TYPE::TEXUI);

	const auto uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_TextureUI",
		"Layer_UI",
		&desc);

	if (!uiHandle)
		return;

	auto* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);
	if (!pUI)
		return;

	pUI->SetAlpha(0.f);
	pUI->SetInputLcok(true);
	pUI->CalcUICoord();

	ACTIVE_BUTTON_INFO info{};
	info.TargetHandle = handle;
	info.UIHandle = *uiHandle;
	info.KeyType = inputKey;
	m_ActiveButtons.push_back(info);

	PlayFadeIn(*uiHandle, 0.f, 0.15f);
}

void UIManager::RemoveActiveButton(CHandle handle, _bool fadeOut)
{
	const auto iter = std::find_if(
		m_ActiveButtons.begin(),
		m_ActiveButtons.end(),
		[handle](const ACTIVE_BUTTON_INFO& info)
		{
			return info.TargetHandle == handle;
		});

	if (iter == m_ActiveButtons.end())
		return;

	const CHandle uiHandle = iter->UIHandle;
	if (auto* pUI = SafeGetOBJ(uiHandle))
	{
		pUI->SetActive(true);
		if (fadeOut)
			PlayFadeOutDelete(uiHandle, 0.f, 0.15f);
		else
			DeleteUIRecursive(uiHandle);
	}

	m_ActiveButtons.erase(iter);
}

void UIManager::UpdateActiveButtons()
{
	auto* pCamera = E::CGameInstance::Get().GetActiveCamera();
	if (!pCamera)
		return;

	const _float2 screenSize = E::CGameInstance::Get().GetClientScreenSize();
	const _matrix view = pCamera->GetView();
	const _matrix proj = pCamera->GetProj();
	const _float2 screenCenter{ screenSize.x * 0.5f, screenSize.y * 0.5f };

	CHandle nearestE{};
	CHandle nearestF{};
	CHandle nearestX{};
	_bool foundNearestE{};
	_bool foundNearestF{};
	_bool foundNearestX{};
	_float nearestEDistanceSq = FLT_MAX;
	_float nearestFDistanceSq = FLT_MAX;
	_float nearestXDistanceSq = FLT_MAX;

	for (auto iter = m_ActiveButtons.begin(); iter != m_ActiveButtons.end();)
	{
		auto* pTarget = E::CGameInstance::Get().GetGameObjectByHandle(iter->TargetHandle);
		auto* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(iter->UIHandle);

		if (!pTarget || pTarget->GetPendingDestroy() || !pUI || pUI->GetPendingDestroy())
		{
			if (pUI && !pUI->GetPendingDestroy())
				DeleteUIRecursive(iter->UIHandle);

			iter = m_ActiveButtons.erase(iter);
			continue;
		}

		_float3 worldPosition = pTarget->GetTransform().GetPosition();
		worldPosition.x += iter->WorldOffset.x;
		worldPosition.y += iter->WorldOffset.y;
		worldPosition.z += iter->WorldOffset.z;

		const _vector clipPosition = XMVector4Transform(
			XMVectorSet(worldPosition.x, worldPosition.y, worldPosition.z, 1.f),
			view * proj);

		_bool visible = XMVectorGetW(clipPosition) > 0.f;
		_float3 screenPosition{};

		if (visible)
		{
			const _vector projected = XMVector3Project(
				XMLoadFloat3(&worldPosition),
				0.f, 0.f,
				screenSize.x, screenSize.y,
				0.f, 1.f,
				proj, view, XMMatrixIdentity());

			XMStoreFloat3(&screenPosition, projected);
			visible = screenPosition.z >= 0.f && screenPosition.z <= 1.f &&
				screenPosition.x >= 0.f && screenPosition.x <= screenSize.x &&
				screenPosition.y >= 0.f && screenPosition.y <= screenSize.y;
		}

		iter->Visible = visible;
		pUI->SetActive(visible);

		if (visible)
		{
			pUI->SetPos({ screenPosition.x, screenPosition.y });
			pUI->CalcUICoord();

			const _float dx = screenPosition.x - screenCenter.x;
			const _float dy = screenPosition.y - screenCenter.y;
			const _float distanceSq = dx * dx + dy * dy;

			if (iter->KeyType == DIK_E && distanceSq < nearestEDistanceSq)
			{
				nearestEDistanceSq = distanceSq;
				nearestE = iter->TargetHandle;
				foundNearestE = true;
			}
			else if (iter->KeyType == DIK_F && distanceSq < nearestFDistanceSq)
			{
				nearestFDistanceSq = distanceSq;
				nearestF = iter->TargetHandle;
				foundNearestF = true;
			}
			else if (iter->KeyType == DIK_X && distanceSq < nearestXDistanceSq)
			{
				nearestXDistanceSq = distanceSq;
				nearestX = iter->TargetHandle;
				foundNearestX = true;
			}
		}

		++iter;
	}

	// The gameplay input is not consumed here. The interaction object can still
	// process the same KeyDown this frame; only the closest visible prompt is removed.
	if (foundNearestE && E::CGameInstance::Get().KeyDown(DIK_E))
		RemoveActiveButton(nearestE);
	if (foundNearestF && E::CGameInstance::Get().KeyDown(DIK_F))
		RemoveActiveButton(nearestF);
	if (foundNearestX && E::CGameInstance::Get().KeyDown(DIK_X))
		RemoveActiveButton(nearestX);
}

void UIManager::AddDialoguePopup(
	const std::string& speaker,
	const std::string& message)
{
	if (speaker.empty() || message.empty())
		return;

	const std::wstring speakerText = StringToWUTF8(speaker + ":");
	const std::wstring messageText = StringToWUTF8(message);
	const _float2 screenSize = E::CGameInstance::Get().GetClientScreenSize();
	const _float centerX = screenSize.x * 0.5f;
	const _float bottomY = screenSize.y - DIALOGUE_BOTTOM_MARGIN;

	const _float speakerWidth = E::CGameInstance::Get().FontMeasureString(
		"Pretendard", speakerText.c_str(), DIALOGUE_FONT_SCALE).x;
	const _float messageWidth = E::CGameInstance::Get().FontMeasureString(
		"Pretendard", messageText.c_str(), DIALOGUE_FONT_SCALE).x;
	const _float textWidth = speakerWidth + DIALOGUE_TEXT_GAP + messageWidth;
	const _float totalWidth = std::clamp(
		textWidth + DIALOGUE_SIDE_PADDING * 2.f,
		DIALOGUE_MIN_WIDTH,
		DIALOGUE_MAX_WIDTH);

	const std::string currentLevel =
		_string("LEVEL_") +
		MagicEnumToStringView(
			static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	CTextureUI::UIOBJECT_DESC backgroundDesc{};
	backgroundDesc.sObjectTag = "DialoguePopupBackground";
	backgroundDesc.Name = "DialoguePopupBackground";
	backgroundDesc.fX = centerX;
	backgroundDesc.fY = bottomY;
	backgroundDesc.fSizeX = totalWidth;
	backgroundDesc.fSizeY = DIALOGUE_BACKGROUND_HEIGHT;
	backgroundDesc.fAlpha = 0.f;
	backgroundDesc.ResWeight = 880;
	backgroundDesc.ResTag = "TEX_UI_T_TextTitle_BG";
	backgroundDesc.UIType = ETOUI(UI_TYPE::TEXUI);

	const auto backgroundHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_TextureUI",
		"Layer_UI",
		&backgroundDesc);
	if (!backgroundHandle)
		return;

	const _float textLeft = centerX - textWidth * 0.5f;
	const _float speakerX = textLeft + speakerWidth * 0.5f;
	const _float messageX = textLeft + speakerWidth +
		DIALOGUE_TEXT_GAP + messageWidth * 0.5f;

	CTextUI::TEXT_DESC speakerDesc{};
	speakerDesc.sObjectTag = "DialoguePopupSpeaker";
	speakerDesc.Name = "DialoguePopupSpeaker";
	speakerDesc.fX = speakerX;
	speakerDesc.fY = bottomY + DIALOGUE_TEXT_Y_OFFSET;
	speakerDesc.fSizeX = DIALOGUE_FONT_SCALE;
	speakerDesc.fSizeY = DIALOGUE_FONT_SCALE;
	speakerDesc.fAlpha = 0.f;
	speakerDesc.ResWeight = 881;
	speakerDesc.UIType = ETOUI(UI_TYPE::TEXT);
	speakerDesc.Alignment = TEXT_ALIGN::CENTER;

	const auto speakerHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_TextBox",
		"Layer_UI",
		&speakerDesc);
	if (!speakerHandle)
	{
		DeleteUIRecursive(*backgroundHandle);
		return;
	}

	CTextUI::TEXT_DESC messageDesc = speakerDesc;
	messageDesc.sObjectTag = "DialoguePopupMessage";
	messageDesc.Name = "DialoguePopupMessage";
	messageDesc.fX = messageX;
	messageDesc.ResWeight = 882;

	const auto messageHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_TextBox",
		"Layer_UI",
		&messageDesc);
	if (!messageHandle)
	{
		DeleteUIRecursive(*backgroundHandle);
		DeleteUIRecursive(*speakerHandle);
		return;
	}

	auto* pBackground = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*backgroundHandle);
	auto* pSpeaker = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*speakerHandle);
	auto* pMessage = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*messageHandle);
	if (!pBackground || !pSpeaker || !pMessage)
	{
		if (pBackground) DeleteUIRecursive(*backgroundHandle);
		if (pSpeaker) DeleteUIRecursive(*speakerHandle);
		if (pMessage) DeleteUIRecursive(*messageHandle);
		return;
	}

	pSpeaker->SetwText(speakerText);
	pSpeaker->SetTextAlignment(TEXT_ALIGN::CENTER);
	pSpeaker->SetColor({ 0.82f, 0.70f, 0.42f });
	pSpeaker->SetInputLcok(true);
	pSpeaker->CalcUICoord();

	pMessage->SetwText(messageText);
	pMessage->SetTextAlignment(TEXT_ALIGN::CENTER);
	pMessage->SetColor({ 1.f, 1.f, 1.f });
	pMessage->SetInputLcok(true);
	pMessage->CalcUICoord();

	pBackground->SetInputLcok(true);
	pBackground->CalcUICoord();

	DIALOGUE_POPUP_INFO popup{};
	popup.BackgroundHandle = *backgroundHandle;
	popup.SpeakerHandle = *speakerHandle;
	popup.MessageHandle = *messageHandle;
	popup.SpeakerWidth = speakerWidth;
	popup.MessageWidth = messageWidth;
	popup.TotalWidth = totalWidth;
	popup.CurrentY = bottomY;
	popup.TargetY = bottomY;
	m_DialoguePopups.push_back(popup);

	if (m_DialoguePopups.size() > DIALOGUE_MAX_COUNT)
	{
		auto& oldest = m_DialoguePopups.front();
		oldest.ElapsedTime = DIALOGUE_HOLD_TIME;
		oldest.FadingOut = true;
	}

	RefreshDialoguePopupLayout();
}

void UIManager::ClearDialoguePopups(_bool immediate)
{
	if (!immediate)
	{
		for (auto& popup : m_DialoguePopups)
		{
			popup.ElapsedTime = DIALOGUE_HOLD_TIME;
			popup.FadingOut = true;
		}
		return;
	}

	for (const auto& popup : m_DialoguePopups)
	{
		if (SafeGetOBJ(popup.BackgroundHandle)) DeleteUIRecursive(popup.BackgroundHandle);
		if (SafeGetOBJ(popup.SpeakerHandle)) DeleteUIRecursive(popup.SpeakerHandle);
		if (SafeGetOBJ(popup.MessageHandle)) DeleteUIRecursive(popup.MessageHandle);
	}
	m_DialoguePopups.clear();
	m_fDialogueTargetWidth = 0.f;
}

void UIManager::RefreshDialoguePopupLayout()
{
	const _float2 screenSize = E::CGameInstance::Get().GetClientScreenSize();
	const _float bottomY = screenSize.y - DIALOGUE_BOTTOM_MARGIN;
	m_fDialogueTargetWidth = 0.f;

	for (const auto& popup : m_DialoguePopups)
		m_fDialogueTargetWidth = std::max(m_fDialogueTargetWidth, popup.TotalWidth);

	for (size_t i = 0; i < m_DialoguePopups.size(); ++i)
	{
		const size_t distanceFromNewest = m_DialoguePopups.size() - 1u - i;
		m_DialoguePopups[i].TargetY = bottomY -
			DIALOGUE_ROW_INTERVAL * static_cast<_float>(distanceFromNewest);
	}
}

void UIManager::UpdateDialoguePopups(_float fTimeDelta)
{
	if (m_DialoguePopups.empty())
		return;

	const _float safeDelta = std::clamp(fTimeDelta, 0.f, 0.05f);
	const _float positionBlend = 1.f - std::exp(-14.f * safeDelta);
	const _float widthBlend = 1.f - std::exp(-12.f * safeDelta);
	const _float2 screenSize = E::CGameInstance::Get().GetClientScreenSize();
	const _float centerX = screenSize.x * 0.5f;
	_bool layoutDirty{};

	for (auto iter = m_DialoguePopups.begin(); iter != m_DialoguePopups.end();)
	{
		auto* pBackground = SafeGetOBJ(iter->BackgroundHandle);
		auto* pSpeaker = SafeGetOBJ(iter->SpeakerHandle);
		auto* pMessage = SafeGetOBJ(iter->MessageHandle);
		if (!pBackground || !pSpeaker || !pMessage)
		{
			if (pBackground) DeleteUIRecursive(iter->BackgroundHandle);
			if (pSpeaker) DeleteUIRecursive(iter->SpeakerHandle);
			if (pMessage) DeleteUIRecursive(iter->MessageHandle);
			iter = m_DialoguePopups.erase(iter);
			layoutDirty = true;
			continue;
		}

		iter->ElapsedTime += safeDelta;
		if (iter->ElapsedTime >= DIALOGUE_HOLD_TIME)
			iter->FadingOut = true;

		if (iter->FadingOut &&
			iter->ElapsedTime >= DIALOGUE_HOLD_TIME + DIALOGUE_FADE_OUT_TIME)
		{
			DeleteUIRecursive(iter->BackgroundHandle);
			DeleteUIRecursive(iter->SpeakerHandle);
			DeleteUIRecursive(iter->MessageHandle);
			iter = m_DialoguePopups.erase(iter);
			layoutDirty = true;
			continue;
		}

		_float alpha = std::clamp(
			iter->ElapsedTime / DIALOGUE_FADE_IN_TIME, 0.f, 1.f);
		if (iter->FadingOut)
		{
			alpha = 1.f - std::clamp(
				(iter->ElapsedTime - DIALOGUE_HOLD_TIME) /
				DIALOGUE_FADE_OUT_TIME,
				0.f, 1.f);
		}

		iter->CurrentY += (iter->TargetY - iter->CurrentY) * positionBlend;
		const _float currentWidth = pBackground->GetSize().x;
		const _float nextWidth = currentWidth +
			(m_fDialogueTargetWidth - currentWidth) * widthBlend;
		pBackground->SetSize({ nextWidth, DIALOGUE_BACKGROUND_HEIGHT });
		pBackground->SetPos({ centerX, iter->CurrentY });
		pBackground->SetAlpha(alpha * 0.75f);
		pBackground->CalcUICoord();

		const _float textWidth = iter->SpeakerWidth +
			DIALOGUE_TEXT_GAP + iter->MessageWidth;
		const _float textLeft = centerX - textWidth * 0.5f;
		pSpeaker->SetPos({
			textLeft + iter->SpeakerWidth * 0.5f,
			iter->CurrentY + DIALOGUE_TEXT_Y_OFFSET });
		pSpeaker->SetAlpha(alpha);
		pSpeaker->CalcUICoord();

		pMessage->SetPos({
			textLeft + iter->SpeakerWidth + DIALOGUE_TEXT_GAP +
				iter->MessageWidth * 0.5f,
			iter->CurrentY + DIALOGUE_TEXT_Y_OFFSET });
		pMessage->SetAlpha(alpha);
		pMessage->CalcUICoord();

		++iter;
	}

	if (layoutDirty)
		RefreshDialoguePopupLayout();
}

void UIManager::ShowNPCSpeechBubble(
	CHandle npcHandle,
	const std::string& message,
	_float duration,
	const _float3& worldOffset)
{
	auto* pTarget = E::CGameInstance::Get().GetGameObjectByHandle(npcHandle);
	if (!pTarget || pTarget->GetPendingDestroy() || message.empty())
		return;

	const std::wstring messageText = StringToWUTF8(message);
	const _float textWidth = E::CGameInstance::Get().FontMeasureString(
		"Pretendard", messageText.c_str(), NPC_SPEECH_FONT_SCALE).x;
	const _float backgroundWidth = textWidth + NPC_SPEECH_SIDE_PADDING * 2.f;
	const _float displayDuration = duration > 0.f ?
		duration : NPC_SPEECH_DEFAULT_DURATION;

	const auto duplicate = std::find_if(
		m_NPCSpeechBubbles.begin(),
		m_NPCSpeechBubbles.end(),
		[npcHandle](const NPC_SPEECH_BUBBLE_INFO& info)
		{
			return info.TargetHandle == npcHandle;
		});

	if (duplicate != m_NPCSpeechBubbles.end())
	{
		auto* pBackground = SafeGetOBJ(duplicate->BackgroundHandle);
		auto* pText = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(
			duplicate->TextHandle);

		if (pBackground && pText)
		{
			pText->SetwText(messageText);
			pBackground->SetSize({ backgroundWidth, NPC_SPEECH_BACKGROUND_HEIGHT });
			duplicate->WorldOffset = worldOffset;
			duplicate->Duration = displayDuration;
			duplicate->ElapsedTime = 0.f;
			duplicate->FadingOut = false;
			pBackground->SetActive(true);
			pText->SetActive(true);
			return;
		}

		if (pBackground)
			DeleteUIRecursive(duplicate->BackgroundHandle);
		else if (pText)
			DeleteUIRecursive(duplicate->TextHandle);
		m_NPCSpeechBubbles.erase(duplicate);
	}

	const std::string currentLevel =
		_string("LEVEL_") +
		MagicEnumToStringView(
			static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	CTextureUI::UIOBJECT_DESC backgroundDesc{};
	backgroundDesc.sObjectTag = "NPCSpeechBubbleBackground";
	backgroundDesc.Name = "NPCSpeechBubbleBackground";
	backgroundDesc.fSizeX = backgroundWidth;
	backgroundDesc.fSizeY = NPC_SPEECH_BACKGROUND_HEIGHT;
	backgroundDesc.fAlpha = 0.f;
	backgroundDesc.ResWeight = 900;
	backgroundDesc.ResTag = "TEX_UI_T_TextTitle_BG";
	backgroundDesc.UIType = ETOUI(UI_TYPE::TEXUI);

	const auto backgroundHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_TextureUI",
		"Layer_UI",
		&backgroundDesc);
	if (!backgroundHandle)
		return;

	CTextUI::TEXT_DESC textDesc{};
	textDesc.sObjectTag = "NPCSpeechBubbleText";
	textDesc.Name = "NPCSpeechBubbleText";
	textDesc.fSizeX = NPC_SPEECH_FONT_SCALE;
	textDesc.fSizeY = NPC_SPEECH_FONT_SCALE;
	textDesc.fAlpha = 1.f;
	textDesc.ResWeight = 901;
	textDesc.UIType = ETOUI(UI_TYPE::TEXT);
	textDesc.Alignment = TEXT_ALIGN::CENTER;

	const auto textHandle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_TextBox",
		"Layer_UI",
		&textDesc);
	if (!textHandle)
	{
		DeleteUIRecursive(*backgroundHandle);
		return;
	}

	auto* pBackground = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(
		*backgroundHandle);
	auto* pText = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(
		*textHandle);
	if (!pBackground || !pText)
	{
		if (pBackground)
			DeleteUIRecursive(*backgroundHandle);
		if (pText)
			DeleteUIRecursive(*textHandle);
		return;
	}

	pBackground->SetInputLcok(true);
	pBackground->SetAlpha(0.f);
	pBackground->SetActive(false);
	pBackground->AddChildren(*textHandle);
	pBackground->CalcUICoord();

	pText->SetwText(messageText);
	pText->SetTextAlignment(TEXT_ALIGN::CENTER);
	pText->SetColor({ 1.f, 1.f, 1.f });
	pText->SetInputLcok(true);
	pText->SetParent(*backgroundHandle);
	pText->SetLocalPos({ 0.f, NPC_SPEECH_TEXT_Y_OFFSET });
	pText->SetAlphaRatio(1.f);
	pText->GetUIInfo().WeightOffset = 1;
	pText->SetActive(false);

	NPC_SPEECH_BUBBLE_INFO bubble{};
	bubble.TargetHandle = npcHandle;
	bubble.BackgroundHandle = *backgroundHandle;
	bubble.TextHandle = *textHandle;
	bubble.WorldOffset = worldOffset;
	bubble.Duration = displayDuration;
	bubble.CurrentScale = 1.f;
	m_NPCSpeechBubbles.push_back(bubble);
}

void UIManager::RemoveNPCSpeechBubble(CHandle npcHandle, _bool fadeOut)
{
	const auto iter = std::find_if(
		m_NPCSpeechBubbles.begin(),
		m_NPCSpeechBubbles.end(),
		[npcHandle](const NPC_SPEECH_BUBBLE_INFO& info)
		{
			return info.TargetHandle == npcHandle;
		});

	if (iter == m_NPCSpeechBubbles.end())
		return;

	if (fadeOut)
	{
		iter->ElapsedTime = iter->Duration;
		iter->FadingOut = true;
		return;
	}

	if (SafeGetOBJ(iter->BackgroundHandle))
		DeleteUIRecursive(iter->BackgroundHandle);
	else if (SafeGetOBJ(iter->TextHandle))
		DeleteUIRecursive(iter->TextHandle);
	m_NPCSpeechBubbles.erase(iter);
}

void UIManager::ClearNPCSpeechBubbles(_bool immediate)
{
	if (!immediate)
	{
		for (auto& bubble : m_NPCSpeechBubbles)
		{
			bubble.ElapsedTime = bubble.Duration;
			bubble.FadingOut = true;
		}
		return;
	}

	for (const auto& bubble : m_NPCSpeechBubbles)
	{
		if (SafeGetOBJ(bubble.BackgroundHandle))
			DeleteUIRecursive(bubble.BackgroundHandle);
		else if (SafeGetOBJ(bubble.TextHandle))
			DeleteUIRecursive(bubble.TextHandle);
	}
	m_NPCSpeechBubbles.clear();
}

void UIManager::CreateOrChangeQuest(const std::string& questText)
{
	if (questText.empty())
	{
		DeleteQuest();
		return;
	}

	auto* root = m_hQuestRoot ? GetSafeUI(*m_hQuestRoot) : nullptr;
	auto* text = m_hQuestText ? E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextBox>(*m_hQuestText) : nullptr;
	auto* targetIcon = m_hQuestTargetIcon ?
		GetSafeUI(*m_hQuestTargetIcon) : nullptr;

	if (!root || !text)
	{
		m_hQuestRoot = std::nullopt;
		m_hQuestText = std::nullopt;
		m_hQuestTargetIcon = std::nullopt;

		const auto roots = LoadPrefab("Quest");
		if (roots.empty())
			return;

		root = FindUIByNameRecursive(roots, "QuestFrame");
		if (auto* textUI = FindUIByNameRecursive(roots, "QuestText"))
		{
			text = E::CGameInstance::Get().
				GetGameObjectByHandleT<CTextBox>(textUI->GetHandle());
		}
		targetIcon = FindUIByNameRecursive(roots, "QuestTargetIcon");
		if (!root || !text)
		{
			for (const CHandle rootHandle : roots)
				DeleteUIRecursive(rootHandle);
			return;
		}

		m_hQuestRoot = root->GetHandle();
		m_hQuestText = text->GetHandle();
		if (targetIcon)
		{
			m_hQuestTargetIcon = targetIcon->GetHandle();
			m_QuestTargetIconBaseLocalPos = {
				targetIcon->GetUIInfo().LocalX,
				targetIcon->GetUIInfo().LocalY
			};
		}
		m_QuestTextBaseLocalPos = {
			text->GetUIInfo().LocalX,
			text->GetUIInfo().LocalY
		};
		m_CurrentQuestText = questText;
		text->SetwText(StringToWUTF8(questText));
		root->SetAlpha(0.f);

		// TextureUI는 최초 APPEAR 처리에서 tween을 초기화한다. 최초 프레임의
		// APPEAR 콜백에서 FadeIn을 시작해야 알파 0에 고정되지 않는다.
		const CHandle rootHandle = *m_hQuestRoot;
		root->Appear = [rootHandle](CUIObject*)
		{
			if (auto* questRoot = GetSafeUI(rootHandle))
			{
				questRoot->SetAlpha(0.f);
				if (!GET_SINGLE(UIManager)->m_bQuestFadeSuppressed)
				{
					GET_SINGLE(UIManager)->PlayFadeIn(
						rootHandle, 0.f, 0.3f);
				}
			}
		};
		return;
	}

	if (m_CurrentQuestText == questText)
		return;

	m_CurrentQuestText = questText;
	auto* tween = text->GetTweenCom();
	if (!tween)
	{
		text->SetwText(StringToWUTF8(questText));
		return;
	}

	tween->ClearTweens();
	const CHandle textHandle = *m_hQuestText;
	const _float baseX = m_QuestTextBaseLocalPos.x;
	const _float baseY = m_QuestTextBaseLocalPos.y;
	constexpr _float shift = 18.f;
	constexpr _float outDuration = 0.2f;
	constexpr _float inDuration = 0.25f;
	const std::wstring nextText = StringToWUTF8(questText);

	tween->PlayTween(
		text->GetAlphaRatio(), 0.f, outDuration,
		[textHandle](_float value)
		{
			if (auto* ui = GetSafeUI(textHandle))
				ui->SetAlphaRatio(value);
		},
		[textHandle, nextText, baseX, baseY]()
		{
			if (auto* textBox = E::CGameInstance::Get().
				GetGameObjectByHandleT<CTextBox>(textHandle))
			{
				textBox->SetwText(nextText);
				textBox->SetLocalPos({ baseX - shift, baseY });
				textBox->SetAlphaRatio(0.f);
				textBox->CalcUICoord();
			}
		}, EEaseType::EaseOutQuad);
	tween->PlayTween(
		baseX, baseX - shift, outDuration,
		[textHandle, baseY](_float value)
		{
			if (auto* ui = GetSafeUI(textHandle))
			{
				ui->SetLocalPos({ value, baseY });
				ui->CalcUICoord();
			}
		}, nullptr, EEaseType::EaseOutQuad);
	tween->PlayTween(
		0.f, 1.f, inDuration,
		[textHandle](_float value)
		{
			if (auto* ui = GetSafeUI(textHandle))
				ui->SetAlphaRatio(value);
		}, nullptr, EEaseType::EaseOutQuad, outDuration);
	tween->PlayTween(
		baseX - shift, baseX, inDuration,
		[textHandle, baseY](_float value)
		{
			if (auto* ui = GetSafeUI(textHandle))
			{
				ui->SetLocalPos({ value, baseY });
				ui->CalcUICoord();
			}
		}, nullptr, EEaseType::EaseOutQuad, outDuration);

	// 퀘스트 문구 옆의 목표 아이콘도 문구 교체 모션과 같은 타이밍으로
	// 함께 빠졌다가 나타나도록 한다.
	if (targetIcon)
	{
		if (auto* iconTween = targetIcon->GetTweenCom())
		{
			iconTween->ClearTweens();
			const CHandle iconHandle = targetIcon->GetHandle();
			const _float iconBaseX = m_QuestTargetIconBaseLocalPos.x;
			const _float iconBaseY = m_QuestTargetIconBaseLocalPos.y;

			iconTween->PlayTween(
				targetIcon->GetAlphaRatio(), 0.f, outDuration,
				[iconHandle](_float value)
				{
					if (auto* ui = GetSafeUI(iconHandle))
						ui->SetAlphaRatio(value);
				},
				[iconHandle, iconBaseX, iconBaseY]()
				{
					if (auto* ui = GetSafeUI(iconHandle))
					{
						ui->SetLocalPos({ iconBaseX - shift, iconBaseY });
						ui->SetAlphaRatio(0.f);
						ui->CalcUICoord();
					}
				}, EEaseType::EaseOutQuad);
			iconTween->PlayTween(
				iconBaseX, iconBaseX - shift, outDuration,
				[iconHandle, iconBaseY](_float value)
				{
					if (auto* ui = GetSafeUI(iconHandle))
					{
						ui->SetLocalPos({ value, iconBaseY });
						ui->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad);
			iconTween->PlayTween(
				0.f, 1.f, inDuration,
				[iconHandle](_float value)
				{
					if (auto* ui = GetSafeUI(iconHandle))
						ui->SetAlphaRatio(value);
				}, nullptr, EEaseType::EaseOutQuad, outDuration);
			iconTween->PlayTween(
				iconBaseX - shift, iconBaseX, inDuration,
				[iconHandle, iconBaseY](_float value)
				{
					if (auto* ui = GetSafeUI(iconHandle))
					{
						ui->SetLocalPos({ value, iconBaseY });
						ui->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, outDuration);
		}
	}
}

void UIManager::DeleteQuest()
{
	if (m_hQuestRoot && GetSafeUI(*m_hQuestRoot))
		PlayFadeOutDelete(*m_hQuestRoot, 0.f, 0.3f);

	m_hQuestRoot = std::nullopt;
	m_hQuestText = std::nullopt;
	m_hQuestTargetIcon = std::nullopt;
	m_CurrentQuestText.clear();
	m_QuestTextBaseLocalPos = {};
	m_QuestTargetIconBaseLocalPos = {};
}

void UIManager::FadeOutQuest(float playtime)
{
	m_bQuestFadeSuppressed = true;
	if (!m_hQuestRoot)
		return;

	const CHandle questHandle = *m_hQuestRoot;
	auto* questRoot = GetSafeUI(questHandle);
	if (!questRoot || !questRoot->GetTweenCom())
		return;

	questRoot->GetTweenCom()->ClearTweens();
	const _float startAlpha = questRoot->GetAlpha();
	questRoot->GetTweenCom()->PlayTween(
		startAlpha, 0.f, std::max(0.f, playtime),
		[questHandle](_float value)
		{
			if (auto* quest = GetSafeUI(questHandle))
				quest->SetAlpha(value);
		}, nullptr, EEaseType::EaseOutQuad);
}

void UIManager::FadeInQuest(float playtime)
{
	m_bQuestFadeSuppressed = false;
	if (!m_hQuestRoot)
		return;

	const CHandle questHandle = *m_hQuestRoot;
	auto* questRoot = GetSafeUI(questHandle);
	if (!questRoot || !questRoot->GetTweenCom())
		return;

	questRoot->GetTweenCom()->ClearTweens();
	const _float startAlpha = questRoot->GetAlpha();
	questRoot->GetTweenCom()->PlayTween(
		startAlpha, 1.f, std::max(0.f, playtime),
		[questHandle](_float value)
		{
			if (auto* quest = GetSafeUI(questHandle))
				quest->SetAlpha(value);
		}, nullptr, EEaseType::EaseOutQuad);
}

void UIManager::UpdateNPCSpeechBubbles(_float fTimeDelta)
{
	if (m_NPCSpeechBubbles.empty())
		return;

	const _float safeDelta = std::clamp(fTimeDelta, 0.f, 0.05f);
	const _float scaleBlend = 1.f - std::exp(
		-NPC_SPEECH_SCALE_SMOOTH_SPEED * safeDelta);
	auto* pCamera = E::CGameInstance::Get().GetActiveCamera();
	const _float2 screenSize = E::CGameInstance::Get().GetClientScreenSize();

	for (auto iter = m_NPCSpeechBubbles.begin();
		iter != m_NPCSpeechBubbles.end();)
	{
		auto* pTarget = E::CGameInstance::Get().GetGameObjectByHandle(
			iter->TargetHandle);
		auto* pBackground = SafeGetOBJ(iter->BackgroundHandle);
		auto* pText = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(
			iter->TextHandle);

		if (!pTarget || pTarget->GetPendingDestroy() ||
			!pBackground || pBackground->GetPendingDestroy() ||
			!pText || pText->GetPendingDestroy())
		{
			if (pBackground && !pBackground->GetPendingDestroy())
				DeleteUIRecursive(iter->BackgroundHandle);
			else if (pText && !pText->GetPendingDestroy())
				DeleteUIRecursive(iter->TextHandle);
			iter = m_NPCSpeechBubbles.erase(iter);
			continue;
		}

		iter->ElapsedTime += safeDelta;
		if (iter->ElapsedTime >= iter->Duration)
			iter->FadingOut = true;

		if (iter->FadingOut &&
			iter->ElapsedTime >= iter->Duration + NPC_SPEECH_FADE_OUT_TIME)
		{
			DeleteUIRecursive(iter->BackgroundHandle);
			iter = m_NPCSpeechBubbles.erase(iter);
			continue;
		}

		_float alpha = std::clamp(
			iter->ElapsedTime / NPC_SPEECH_FADE_IN_TIME, 0.f, 1.f);
		if (iter->FadingOut)
		{
			alpha = 1.f - std::clamp(
				(iter->ElapsedTime - iter->Duration) /
				NPC_SPEECH_FADE_OUT_TIME,
				0.f, 1.f);
		}

		_bool visible = pCamera != nullptr;
		_float3 screenPosition{};
		_float targetScale = NPC_SPEECH_MIN_SCALE;

		if (visible)
		{
			_float3 worldPosition = pTarget->GetTransform().GetPosition();
			worldPosition.x += iter->WorldOffset.x;
			worldPosition.y += iter->WorldOffset.y;
			worldPosition.z += iter->WorldOffset.z;

			const _matrix view = pCamera->GetView();
			const _matrix proj = pCamera->GetProj();
			const _vector clipPosition = XMVector4Transform(
				XMVectorSet(
					worldPosition.x,
					worldPosition.y,
					worldPosition.z,
					1.f),
				view * proj);
			visible = XMVectorGetW(clipPosition) > 0.f;

			if (visible)
			{
				const _vector projected = XMVector3Project(
					XMLoadFloat3(&worldPosition),
					0.f, 0.f,
					screenSize.x, screenSize.y,
					0.f, 1.f,
					proj, view, XMMatrixIdentity());
				XMStoreFloat3(&screenPosition, projected);

				visible = screenPosition.z >= 0.f && screenPosition.z <= 1.f &&
					screenPosition.x >= 0.f && screenPosition.x <= screenSize.x &&
					screenPosition.y >= 0.f && screenPosition.y <= screenSize.y;
			}

			const _float3 cameraPosition = pCamera->GetTransform().GetPosition();
			const _float dx = worldPosition.x - cameraPosition.x;
			const _float dy = worldPosition.y - cameraPosition.y;
			const _float dz = worldPosition.z - cameraPosition.z;
			const _float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
			_float distanceRatio = std::clamp(
				(distance - NPC_SPEECH_NEAR_DISTANCE) /
				(NPC_SPEECH_FAR_DISTANCE - NPC_SPEECH_NEAR_DISTANCE),
				0.f, 1.f);
			distanceRatio = distanceRatio * distanceRatio *
				(3.f - 2.f * distanceRatio);
			targetScale = NPC_SPEECH_MAX_SCALE +
				(NPC_SPEECH_MIN_SCALE - NPC_SPEECH_MAX_SCALE) * distanceRatio;
		}

		iter->CurrentScale +=
			(targetScale - iter->CurrentScale) * scaleBlend;
		pBackground->SetActive(visible);
		pText->SetActive(visible);

		if (visible)
		{
			pBackground->SetPos({ screenPosition.x, screenPosition.y });
			pBackground->SetScaleRatio(iter->CurrentScale);
			pBackground->SetAlpha(alpha);
			pBackground->CalcUICoord();
			pText->SetAlphaRatio(1.f);
		}

		++iter;
	}
}

void UIManager::CreateChoiceUI(
	const std::vector<std::string>& choices,
	std::function<void(size_t)> onSelected)
{
	ClearChoiceUI(true);
	if (choices.empty())
		return;

	constexpr _float CHOICE_INTERVAL_Y = 54.f;
	m_iSelectedDialogueChoice = 0u;
	m_bDialogueChoiceActive = true;
	m_OnDialogueChoiceSelected = std::move(onSelected);

	for (size_t index = 0; index < choices.size(); ++index)
	{
		const auto roots = LoadPrefab("Intersection");
		if (roots.empty())
			continue;

		auto* root = GetSafeUI(roots.front());
		if (!root)
			continue;

		DIALOGUE_CHOICE_UI_INFO info{};
		info.RootHandle = root->GetHandle();

		const _float2 basePosition = root->GetPos();
		root->SetPos({
			basePosition.x,
			basePosition.y + CHOICE_INTERVAL_Y * static_cast<_float>(index) });
		root->CalcUICoord();
		info.BaseRootPosition = root->GetPos();
		info.RootScaleRatio = std::max(0.001f, root->GetScaleRatio());

		if (auto* frameU = FindUIByNameRecursive(roots, "FrameU"))
		{
			info.FrameUHandle = frameU->GetHandle();
			const auto& frameInfo = frameU->GetUIInfo();
			info.BaseFrameULocalPosition = { frameInfo.LocalX, frameInfo.LocalY };
			info.SelectedFrameUAlphaRatio = frameU->GetAlphaRatio();
		}
		if (auto* frameD = FindUIByNameRecursive(roots, "FrameD"))
		{
			info.FrameDHandle = frameD->GetHandle();
			const auto& frameInfo = frameD->GetUIInfo();
			info.BaseFrameDLocalPosition = { frameInfo.LocalX, frameInfo.LocalY };
			info.SelectedFrameDAlphaRatio = frameD->GetAlphaRatio();
		}
		if (auto* fade = FindUIByNameRecursive(roots, "Fade"))
		{
			info.FadeHandle = fade->GetHandle();
			const auto& fadeInfo = fade->GetUIInfo();
			info.BaseFadeLocalPosition = { fadeInfo.LocalX, fadeInfo.LocalY };
			info.SelectedFadeAlphaRatio = fade->GetAlphaRatio();
		}
		if (auto* text = Engine::Cast<CTextBox>(
			FindUIByNameRecursive(roots, "Text")))
		{
			info.TextHandle = text->GetHandle();
			text->SetwText(StringToWUTF8(choices[index]));
		}

		m_DialogueChoiceUIs.push_back(info);
	}

	if (m_DialogueChoiceUIs.empty())
	{
		m_bDialogueChoiceActive = false;
		m_OnDialogueChoiceSelected = nullptr;
		return;
	}

	// 최초 생성 시에는 첫 선택 상태를 즉시 반영한다. 로드 직후의
	// APPEAR 처리에서 Tween이 초기화되므로 방향키 변경부터 모션을 사용한다.
	RefreshDialogueChoiceVisuals(false);
}

void UIManager::ClearChoiceUI(_bool immediate)
{
	for (const auto& choice : m_DialogueChoiceUIs)
	{
		auto* root = GetSafeUI(choice.RootHandle);
		if (!root)
			continue;

		if (immediate)
			DeleteUIRecursive(choice.RootHandle);
		else
			PlayFadeOutDelete(choice.RootHandle, 0.f, 0.2f);
	}

	m_DialogueChoiceUIs.clear();
	m_OnDialogueChoiceSelected = nullptr;
	m_iSelectedDialogueChoice = 0u;
	m_bDialogueChoiceActive = false;
}

void UIManager::RefreshDialogueChoiceVisuals(_bool animate)
{
	constexpr _float SELECT_MOVE_X = 12.f;
	constexpr _float SELECT_DURATION = 0.18f;

	for (size_t index = 0; index < m_DialogueChoiceUIs.size(); ++index)
	{
		const auto& choice = m_DialogueChoiceUIs[index];
		const _bool selected = index == m_iSelectedDialogueChoice;
		const _float rootTargetX = choice.BaseRootPosition.x +
			(selected ? SELECT_MOVE_X : 0.f);
		const _float localCompensation = selected ?
			SELECT_MOVE_X / choice.RootScaleRatio : 0.f;

		if (auto* root = GetSafeUI(choice.RootHandle))
		{
			if (!animate || !root->GetTweenCom())
			{
				root->SetPos({ rootTargetX, choice.BaseRootPosition.y });
				root->CalcUICoord();
			}
			else
			{
				auto* tween = root->GetTweenCom();
				tween->ClearTweens();
				tween->PlayTween(
					root->GetPos().x, rootTargetX, SELECT_DURATION,
					[handle = choice.RootHandle,
					 baseY = choice.BaseRootPosition.y](_float value)
					{
						if (auto* target = GetSafeUI(handle))
						{
							target->SetPos({ value, baseY });
							target->CalcUICoord();
						}
					}, nullptr, EEaseType::EaseOutQuad);
			}
		}

		const auto animateFixedChild =
			[animate, localCompensation](
				CHandle handle,
				const _float2& baseLocalPosition,
				_float targetAlphaRatio)
			{
				auto* child = GetSafeUI(handle);
				if (!child)
					return;

				const _float targetLocalX =
					baseLocalPosition.x - localCompensation;
				if (!animate || !child->GetTweenCom())
				{
					child->SetLocalPos({ targetLocalX, baseLocalPosition.y });
					child->SetAlphaRatio(targetAlphaRatio);
					return;
				}

				auto* tween = child->GetTweenCom();
				tween->ClearTweens();
				const _float startLocalX = child->GetUIInfo().LocalX;
				const _float startAlphaRatio = child->GetAlphaRatio();
				tween->PlayTween(
					startLocalX, targetLocalX, SELECT_DURATION,
					[handle, baseY = baseLocalPosition.y](_float value)
					{
						if (auto* target = GetSafeUI(handle))
							target->SetLocalPos({ value, baseY });
					}, nullptr, EEaseType::EaseOutQuad);
				tween->PlayTween(
					startAlphaRatio, targetAlphaRatio, SELECT_DURATION,
					[handle](_float value)
					{
						if (auto* target = GetSafeUI(handle))
							target->SetAlphaRatio(value);
					}, nullptr, EEaseType::EaseOutQuad);
			};

		animateFixedChild(
			choice.FrameUHandle, choice.BaseFrameULocalPosition,
			selected ? choice.SelectedFrameUAlphaRatio : 0.f);
		animateFixedChild(
			choice.FrameDHandle, choice.BaseFrameDLocalPosition,
			selected ? choice.SelectedFrameDAlphaRatio : 0.f);
		animateFixedChild(
			choice.FadeHandle, choice.BaseFadeLocalPosition,
			selected ? choice.SelectedFadeAlphaRatio : 0.f);
	}
}

void UIManager::UpdateDialogueChoiceUI()
{
	if (!m_bDialogueChoiceActive || m_DialogueChoiceUIs.empty())
		return;

	const size_t choiceCount = m_DialogueChoiceUIs.size();
	_bool selectionChanged{};

	if (E::CGameInstance::Get().KeyDown(DIK_UP))
	{
		m_iSelectedDialogueChoice = m_iSelectedDialogueChoice == 0u ?
			choiceCount - 1u : m_iSelectedDialogueChoice - 1u;
		selectionChanged = true;
	}
	else if (E::CGameInstance::Get().KeyDown(DIK_DOWN))
	{
		m_iSelectedDialogueChoice =
			(m_iSelectedDialogueChoice + 1u) % choiceCount;
		selectionChanged = true;
	}

	if (selectionChanged)
		RefreshDialogueChoiceVisuals();

	if (!E::CGameInstance::Get().KeyDown(DIK_SPACE))
		return;

	const size_t selectedIndex = m_iSelectedDialogueChoice;
	if (selectedIndex < m_DialogueChoiceUIs.size())
	{
		constexpr _float CONFIRM_MOVE_X = 24.f;
		constexpr _float CONFIRM_DURATION = 0.2f;
		const CHandle selectedRoot =
			m_DialogueChoiceUIs[selectedIndex].RootHandle;
		if (auto* root = GetSafeUI(selectedRoot))
		{
			const _float startX = root->GetPos().x;
			const _float fixedY = root->GetPos().y;
			if (auto* tween = root->GetTweenCom())
			{
				tween->ClearTweens();
				tween->PlayTween(
					startX,
					startX + CONFIRM_MOVE_X,
					CONFIRM_DURATION,
					[selectedRoot, fixedY](_float value)
					{
						if (auto* target = GetSafeUI(selectedRoot))
						{
							target->SetPos({ value, fixedY });
							target->CalcUICoord();
						}
					}, nullptr, EEaseType::EaseOutQuad);
			}
		}
	}

	auto callback = std::move(m_OnDialogueChoiceSelected);
	ClearChoiceUI(false);
	if (callback)
		callback(selectedIndex);
}

std::optional<CHandle> UIManager::RootUIPicking()
{
	std::optional<CHandle> targetHandle = std::nullopt;
	for (auto uiHandle : rootUIHandles)
	{
		if (nullptr == E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(uiHandle))
			continue;

		CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(uiHandle);
		const UI_INFO& pInfo = pUI->GetUIInfo();

		if (pUI->GetWorldSpace())
			continue;

		if (PtInRect(pInfo, pUI->GetScaleRatio()))
		{
			if (std::nullopt == targetHandle)
				targetHandle = uiHandle;
			else
			{
				if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*targetHandle))
				{
					CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*targetHandle);
					const UI_INFO& targetInfo = targetUI->GetUIInfo();

					if (pInfo.Weight > targetInfo.Weight)
						targetHandle = uiHandle;
				}
			}
		}
	}

	return targetHandle;
}

_bool UIManager::IsPointerOverInteractiveUI()
{
	const auto IsInteractiveHit = [this](const auto& Self, CHandle hUI) -> _bool
	{
		auto* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(hUI);
		if (!pUI || !pUI->GetActive() || !pUI->GetVisible() || pUI->GetWorldSpace())
			return false;

		// 버튼인 자식 UI까지 검사해야 전체 화면 HUD 루트가 입력을 가로채지 않는다.
		for (const CHandle hChild : pUI->GetChildren())
		{
			if (Self(Self, hChild))
				return true;
		}

		return pUI->HasInteractiveButton() &&
			PtInRect(pUI->GetUIInfo(), pUI->GetScaleRatio());
	};

	for (const CHandle hRoot : rootUIHandles)
	{
		if (IsInteractiveHit(IsInteractiveHit, hRoot))
			return true;
	}
	return false;
}

_bool UIManager::PtInRect(const UI_INFO& selectInfo, _float scaleRatio)
{
	_float2 mousePos = GetUIInteractionMousePosition();

	_float2 origin = { selectInfo.fX, selectInfo.fY };
	_float2 size = { selectInfo.SizeX * scaleRatio, selectInfo.SizeY * scaleRatio };

	if (selectInfo.UIType == ETOUI(UI_TYPE::TEXT))
	{
		size = { selectInfo.SizeX * 50.f, selectInfo.SizeY * 50.f  };
		origin = { selectInfo.fX + size.x * 0.5f, selectInfo.fY + size.y * 0.5f};
	}


	_float2 minPos =
	{
		origin.x - size.x * 0.5f,
		origin.y - size.y * 0.5f
	};

	_float2 maxPos =
	{
		origin.x + size.x * 0.5f,
		origin.y + size.y * 0.5f
	};

	if (mousePos.x >= minPos.x &&
		mousePos.x <= maxPos.x &&
		mousePos.y >= minPos.y &&
		mousePos.y <= maxPos.y)
	{
		return true;
	}

	return false;
}

std::vector<CHandle> UIManager::LoadPrefab(std::string name, std::string g_BasePath)
{
	const std::vector<CHandle> roots = LoadPrefabFiltered(name, g_BasePath, {});
	if (CWandShop::IsPagePrefab(name))
		m_WandShop.RegisterLoadedPage(name, roots);
	return roots;
}

std::vector<CHandle> UIManager::LoadPrefabFiltered(
	const std::string& name,
	const std::string& basePath,
	const std::function<_bool(const nlohmann::ordered_json&)>& predicate)
{
	m_vLoadPrefabRoot.clear();
	uint32_t num = E::CGameInstance::Get().GetCurrentLevelID();
	if (num > 100)
		m_CurrentLevel = "LEVEL_LOADING";
	else
		m_CurrentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	char path[256] = "";
	strcpy_s(path, sizeof(path), basePath.c_str());
	strcat_s(path, sizeof(path), name.c_str());
	strcat_s(path, sizeof(path), ".json");

	std::ifstream file(path);

	if (!file.is_open())
	{
		/* ---- 광윤 수정 ---- */
		std::string MSGBoxText = "Cannot Open json" + basePath + name + ".json";
		MessageBoxA(NULL, MSGBoxText.c_str(), "System Message", MB_OK);
		/* ------------------- */
		return m_vLoadPrefabRoot;
	}

	nlohmann::ordered_json root;
	file >> root;
	file.close();

	for (const auto& obj : root["UI"])
	{
		if (predicate && !predicate(obj))
			continue;
		LoadUIRecursive(obj, nullptr);
	}

	if (m_bWandShopWorldMode)
	{
		// Selection/hover effects used by the shop may come from the normal
		// prefab directory.  While the world shop owns the interaction, route
		// every prefab it creates into the same RTT instead of screen UI.
		for (const CHandle rootHandle : m_vLoadPrefabRoot)
			SetRenderGroupRecursive(rootHandle, E::RENDERGROUP::UI3D);
	}

	return m_vLoadPrefabRoot;
}

void UIManager::OpenWandShopPage(uint32_t pageIndex)
{
	m_WandShop.OpenPage(*this, pageIndex);
}

void UIManager::OpenWandShop()
{
	// The first full load owns the common frame and navigation controls.
	// Page changes afterwards are handled by CWandShop without recreating them.
	if (m_WandShop.IsOpen())
		return;
	m_bWandShopWorldMode = false;
	E::CGameInstance::Get().ClearUI3DPanel();

	CGeneralButton::ResetWandShopSelection();
	LoadPrefab("ShopWand1", "./Resources/SampleClient/UIData/RTT/");
	m_WandShop.CreatePurchasePrompt();
}

void UIManager::OpenWandShopWorld(
	CHandle targetHandle,
	const _float3& positionOffset,
	const _float3& rotationOffsetDegrees,
	_float panelScale)
{
	auto* targetObject = E::CGameInstance::Get().
		GetGameObjectByHandle(targetHandle);
	if (!targetObject)
		return;

	if (m_WandShop.IsOpen())
	{
		// Once spawned in world space, keep the original panel transform.
		// This also guards against an input implementation reporting F4 for
		// more than one frame while the key is held.
		if (m_bWandShopWorldMode)
			return;
		m_WandShop.Close(*this);
	}

	constexpr _float PANEL_WIDTH = 9.6f;
	constexpr _float PANEL_HEIGHT = 5.4f;
	constexpr _float MIN_AXIS_LENGTH_SQ = 0.0001f;

	auto& targetTransform = targetObject->GetTransform();
	_vector targetRight = targetTransform.GetState(STATE::RIGHT);
	_vector targetUp = targetTransform.GetState(STATE::UP);
	_vector targetLook = targetTransform.GetState(STATE::LOOK);

	if (XMVectorGetX(XMVector3LengthSq(targetRight)) < MIN_AXIS_LENGTH_SQ)
		targetRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(targetUp)) < MIN_AXIS_LENGTH_SQ)
		targetUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(targetLook)) < MIN_AXIS_LENGTH_SQ)
		targetLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);

	targetRight = XMVector3Normalize(targetRight);
	targetUp = XMVector3Normalize(targetUp);
	targetLook = XMVector3Normalize(targetLook);

	const _vector panelPosition =
		targetTransform.GetLoadedPostion() +
		targetRight * positionOffset.x +
		targetUp * positionOffset.y +
		targetLook * positionOffset.z;

	// The panel front faces the target's backward direction by default.
	// Rotation offsets are specified in degrees and applied X -> Z -> Y.
	const _matrix targetPanelPose{
		XMVectorSetW(targetRight, 0.f),
		XMVectorSetW(targetUp, 0.f),
		XMVectorSetW(-targetLook, 0.f),
		XMVectorSetW(panelPosition, 1.f)
	};
	const _matrix rotationOffset =
		XMMatrixRotationX(XMConvertToRadians(rotationOffsetDegrees.x)) *
		XMMatrixRotationZ(XMConvertToRadians(rotationOffsetDegrees.z)) *
		XMMatrixRotationY(XMConvertToRadians(rotationOffsetDegrees.y));
	const _float safePanelScale = std::clamp(panelScale, 0.05f, 2.f);
	m_vWandShopPanelWorldScale = {
		PANEL_WIDTH * safePanelScale,
		PANEL_HEIGHT * safePanelScale
	};
	XMStoreFloat3(&m_vWandShopPanelWorldPosition, panelPosition);
	const _matrix panelWorld =
		XMMatrixScaling(
			m_vWandShopPanelWorldScale.x,
			m_vWandShopPanelWorldScale.y,
			1.f) *
		rotationOffset * targetPanelPose;

	_float4x4 storedPanelWorld{};
	XMStoreFloat4x4(&storedPanelWorld, panelWorld);

	m_bWandShopWorldMode = true;
	m_WandShopPanelWorld = storedPanelWorld;
	// Replace the initial NPC-relative rotation immediately with an upright
	// billboard facing whichever camera is currently active.
	UpdateWandShopWorldBillboard();
	// The placement is now confirmed, so use the scene depth buffer and let
	// walls/props occlude the physical panel naturally.
	CGeneralButton::ResetWandShopSelection();
	LoadPrefab("ShopWand1", "./Resources/SampleClient/UIData/RTT/");
	m_WandShop.CreatePurchasePrompt();
}

void UIManager::CloseWandShop()
{
	m_WandShop.Close(*this);
	m_bWandShopWorldMode = false;
	E::CGameInstance::Get().ClearUI3DPanel();
}

E::CUIObject* UIManager::LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent)
{
	int uiType = obj["UiType"];
	const std::string objectName = obj.value("Name", std::string{});
	const _bool legacyWandCoreCard =
		uiType == ETOUI(UI_TYPE::TEXUI) &&
		(objectName == "DragonWandCore" ||
			objectName == "UniCornWandCore" ||
			objectName == "PheonixWandCore");
	if (legacyWandCoreCard)
		uiType = ETOUI(UI_TYPE::GENERAL_BUTTON);

	E::CUIObject* pUI = nullptr;

	E::CUIObject::UIOBJECT_DESC Desc{};
	std::optional<CHandle> uiHandle = std::nullopt;

	Desc.sObjectTag = objectName;

	int EffectType = obj["UI_EFFECT_TYPE"];

	switch (uiType)
	{
	case ETOUI(UI_TYPE::TEXUI):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);

		if (EffectType == ETOUI(UI_EFFECT_TYPE::HOVER))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectHovered(uiHandle);
			}
		}
		else if (EffectType == ETOUI(UI_EFFECT_TYPE::CLICK))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectClicked(uiHandle);
			}
		}
		pUI->SetUIType(ETOUI(UI_TYPE::TEXUI));
		break;
	case ETOUI(UI_TYPE::SHORTCUT_ICON):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::SHORTCUT_ICON));
		break;
	case ETOUI(UI_TYPE::DISOLVE):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::DISOLVE));
		break;
	case ETOUI(UI_TYPE::FLIPBOOK):
	{
		FLIP_INFO loadedFlipInfo{};
		LoadFlipInfoCompatible(obj, loadedFlipInfo);
		CFlipbookUI::FLIPBOOK_DESC flipDesc{};
		flipDesc.sObjectTag = Desc.sObjectTag;
		flipDesc.cellsize = loadedFlipInfo.cellsize;
		flipDesc.TotalFrame = loadedFlipInfo.TotalFrame;
		flipDesc.Columns = loadedFlipInfo.Columns;
		flipDesc.Rows = loadedFlipInfo.Rows;
		flipDesc.Padding = static_cast<uint32_t>(loadedFlipInfo.Padding);
		flipDesc.Duration = loadedFlipInfo.Duration;
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_EffectUI", "Layer_UI", &flipDesc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CEffectUI>(*uiHandle);
		{
			FLIP_INFO& flipInfo = static_cast<CEffectUI*>(pUI)->GetFlipInfo();
			flipInfo = loadedFlipInfo;
		}

		if (EffectType == ETOUI(UI_EFFECT_TYPE::HOVER))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectHovered(uiHandle);
			}
		}
		else if (EffectType == ETOUI(UI_EFFECT_TYPE::CLICK))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectClicked(uiHandle);
			}
		}
		break;
	}
	case ETOUI(UI_TYPE::TEXT):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextBox", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*uiHandle);
		{
			TEXT_INFO& textInfo = static_cast<CTextBox*>(pUI)->GetTextInfo();
			textInfo.Text = StringToWUTF8(
				obj.value("Text", std::string{}));
			textInfo.Alignment = LoadTextAlignmentCompatible(obj);
			textInfo.FontType = LoadTextFontTypeCompatible(obj);
		}
		break;
	case ETOUI(UI_TYPE::BUTTON):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_Button", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CButton>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::GENERAL_BUTTON):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_GeneralButton", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CGeneralButton>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::NINE_SLICE):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::NINE_SLICE));
		break;
	case ETOUI(UI_TYPE::SPELLMETER):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_SpellMeter", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CSpellMeter>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::HPBAR):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_HPBar", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CHPBar>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::HPFILL):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_HPBar", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CHPBar>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::HPFILL));
		break;
	case ETOUI(UI_TYPE::LEFTHPFILL):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_HPBar", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CHPBar>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::LEFTHPFILL));
		break;
	case ETOUI(UI_TYPE::MINIMAP):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_MiniMap", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CMiniMap>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::MINIMAP));
		break;
	case ETOUI(UI_TYPE::SPELLBTN):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_Button", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CButton>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::GAMEOVERMASK):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_GameOverMask", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CGameOverMask>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::VIDEOOBJ):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_VideoObject", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CVideoObject>(*uiHandle);
		break;
	default:
		break;
	}

	if (pUI == nullptr)
		return nullptr;

	// Mark every shop object at creation time.  This avoids a one-frame
	// registration in the normal screen UI queue and also covers children
	// and dynamically loaded shop effects without relying on a later walk.
	if (m_bWandShopWorldMode)
		pUI->SetRenderGroupOverride(E::RENDERGROUP::UI3D);

	if (obj.contains("ScaleRatio"))
		pUI->SetScaleRatio(obj["ScaleRatio"]);
	if (obj.contains("LocalScaleRatio"))
		pUI->SetLocalScaleRatio(obj["LocalScaleRatio"]);

	if (parent == nullptr)
	{
		m_vLoadPrefabRoot.push_back(pUI->GetHandle());
	}

	UI_INFO& uiInfo = static_cast<CUIObject*>(pUI)->GetUIInfo();

	uiInfo.EffectType = obj["UI_EFFECT_TYPE"];
	uiInfo.Name = obj["Name"];

	uiInfo.SizeX = obj["SizeX"];
	uiInfo.SizeY = obj["SizeY"];

	uiInfo.Alpha = obj["Alpha"];
	uiInfo.AlphaRatio = obj["AlphaRatio"];

	uiInfo.Weight = obj["Weight"];
	uiInfo.WeightOffset = obj["WeightOffset"];

	uiInfo.LocalX = obj["LocalX"];
	uiInfo.LocalY = obj["LocalY"];

	uiInfo.WidthRatioX = obj["WidthRatioX"];
	uiInfo.WidthRatioY = obj["WidthRatioY"];
	uiInfo.FlipX = obj.value("FlipX", false);
	uiInfo.FlipY = obj.value("FlipY", false);

	uiInfo.Restag = obj["ResTag"];

	uiInfo.Rot = obj["Rot"];
	uiInfo.LocalRot = obj["LocalRot"];

	auto color = obj["Color"];
	uiInfo.Color = { color[0], color[1], color[2] };

	if (auto* button = E::CGameInstance::Get().GetGameObjectByHandleT<CGeneralButton>(pUI->GetHandle()))
	{
		button->RefreshBaseScale();
		button->SetButtonType(static_cast<GENERAL_BUTTON_TYPE>(
			obj.value("ButtonType", static_cast<uint32_t>(
				legacyWandCoreCard ? GENERAL_BUTTON_TYPE::WAND_CORE_CARD :
					GENERAL_BUTTON_TYPE::DEFAULT))));
		button->SetCommandParameter(obj.value("CommandParameter", std::string{}));
	}
	if (uiType == ETOUI(UI_TYPE::NINE_SLICE))
	{
		if (auto* nineSlice = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(pUI->GetHandle()))
		{
			_float4 margins{};
			if (obj.contains("NineSliceMargins") && obj["NineSliceMargins"].is_array() && obj["NineSliceMargins"].size() >= 4)
			{
				const auto& savedMargins = obj["NineSliceMargins"];
				margins = { savedMargins[0], savedMargins[1], savedMargins[2], savedMargins[3] };
			}
			nineSlice->SetNineSliceMargins(margins);
		}
	}

	UI_EVENT& eventInfo = pUI->GetUIEvent();

	eventInfo.ClickFunc = obj.value("ClickFunc", "");
	eventInfo.ClickAction = obj.value("ClickAction", "");
	eventInfo.EnterAction = obj.value("EnterAction", "");
	eventInfo.ExitAction = obj.value("ExitAction", "");
	eventInfo.AppearAction = obj.value("AppearAction", "");
	eventInfo.DisappearAction = obj.value("DisappearAction", "");

	auto bindAction = [](const std::string& actionStr, std::function<void(CUIObject*)>& targetFunc) {
		if (!actionStr.empty() && actionStr != "None") {
			targetFunc = GET_SINGLE(UIManager)->GetAction(actionStr);
		}
	};

	if (uiInfo.UIType != ETOUI(UI_TYPE::GENERAL_BUTTON))
	{
		bindAction(eventInfo.ClickAction, pUI->OnClicked);
		bindAction(eventInfo.EnterAction, pUI->OnHoverEnter);
		bindAction(eventInfo.ExitAction, pUI->OnHoverExit);
		bindAction(eventInfo.AppearAction, pUI->Appear);
		bindAction(eventInfo.DisappearAction, pUI->Disappear);

		if(!eventInfo.ClickFunc.empty() && eventInfo.ClickFunc != "None")
			pUI->OnClickedAction = GET_SINGLE(UIManager)->GetFunc(eventInfo.ClickFunc);
	}


	if (parent == nullptr)
	{
		m_rootHandle = uiHandle;

		uiInfo.fX = obj["X"];
		uiInfo.fY = obj["Y"];
	}
	else
	{
		pUI->SetParent(parent->GetHandle());
		parent->AddChildren(pUI->GetHandle());

		uiInfo.LocalX = obj["LocalX"];
		uiInfo.LocalY = obj["LocalY"];
	}

	// 부모 기준으로 다시 계산
	if (obj.contains("IsWorldSpace"))
	{
		bool isWorldSpace = obj["IsWorldSpace"];
		pUI->SetWorldSpace(isWorldSpace);

		if (isWorldSpace && obj.contains("WorldPos"))
		{
			auto posArr = obj["WorldPos"];
			_float3 loadedPos = { posArr[0], posArr[1], posArr[2] };

			// Transform에 3D 월드 좌표 적용 (XMLoadFloat3 사용)
			pUI->GetTransform().SetPosition(XMLoadFloat3(&loadedPos));
		}

		if(!isWorldSpace)
			pUI->CalcUICoord();
	}
	else
		pUI->CalcUICoord();
	

	for (const auto& child : obj["Children"])
	{
		LoadUIRecursive(child, pUI);
	}

	return pUI;
}

void UIManager::DeleteUIRecursive(std::optional<CHandle> targetHandle)
{
	Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetHandle);

	std::vector<CHandle>  childHandles = targetUI->GetChildren();

	for (auto childHandle : childHandles)
	{
		DeleteUIRecursive(childHandle);
	}

	if (targetUI->GetParent())
	{
		Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetUI->GetParent());

		if (nullptr != parentUI)
			parentUI->DeleteChild(targetUI->GetHandle());
	}

	targetUI->SetPendingDestroyCascade();

	return;
}

void UIManager::PlayFadeOutDelete(CHandle pHandle, float delay,
	float playtime, std::function<void()> onComplete)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	const _float alpha = pBtn->GetAlpha();

	pTween->PlayTween(alpha, 0.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, [this, pHandle, onComplete = std::move(onComplete)]() {
			if (auto pObj = GetSafeUI(pHandle))
				DeleteUIRecursive(pHandle);
			if (m_hScreenFade && *m_hScreenFade == pHandle)
				m_hScreenFade.reset();
			if (onComplete)
				onComplete();
			}, EEaseType::EaseOutQuad, delay);
}

void UIManager::PlayScaleDown(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	_float scale = pBtn->GetSize().x;

	pTween->PlayTween(scale, scale * 0.5f, playtime,
		[pHandle](float currentValue) {
			if (auto pObj = GetSafeUI(pHandle))
			{
				pObj->GetUIInfo().SizeX = currentValue;
				pObj->CalcUICoord();
			}
		},nullptr, EEaseType::EaseOutQuad, delay);
}

void UIManager::PlayPosUP(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	_float2 originPos = pBtn->GetPos();

	pTween->PlayTween(originPos.y, originPos.y - 20.f, playtime,
		[pHandle](float currentValue) {
			if (auto pObj = GetSafeUI(pHandle))
			{
				pObj->GetUIInfo().fY = currentValue;
				pObj->CalcUICoord();
			}
		}, nullptr, EEaseType::Linear, delay);
}

void UIManager::PlayFadeIn(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();
	_float scaleRatio = pBtn->GetScaleRatio();

	pTween->PlayTween(0.8f, scaleRatio, playtime,
		[pHandle](float currentValue) {
			if (auto pObj = GetSafeUI(pHandle))
			{
				pObj->SetScaleRatio(currentValue);
				pObj->CalcUICoord();
			}
		}, nullptr, EEaseType::EaseOutQuad, delay);

	pTween->PlayTween(0.f, 1.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, delay);
}

void UIManager::PlayOnlyFadeIn(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	const _float alpha = pBtn->GetAlpha();

	pTween->PlayTween(alpha, 1.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, delay);
}

void UIManager::PlayFadeOutAll2DUI(float delay, float playtime)
{
	UpdateRootUIHandles();

	for (const CHandle handle : rootUIHandles)
	{
		auto* pUI = GetSafeUI(handle);
		if (!pUI || !pUI->GetActive() || !pUI->GetVisible() ||
			pUI->GetResolvedRenderGroup() != RENDERGROUP::UI ||
			pUI->GetUIInfo().UIType == ETOUI(UI_TYPE::CURSOR))
			continue;

		// FadeIn 때 복원할 각 UI의 원래 상태를 FadeOut 시작 시점에 보관한다.
		// 대화 시작 FadeOut 뒤 선택창 FadeOut처럼 중첩 호출되어도 최초의
		// 표시 상태를 보존한다.
		m_2DUIRestoreAlpha.try_emplace(handle, pUI->GetAlpha());
		m_2DUIRestoreInputLock.try_emplace(handle, pUI->GetInputLcok());
		pUI->SetInputLcok(true);

		if (auto* pTween = pUI->GetTweenCom())
		{
			const _float startAlpha = pUI->GetAlpha();
			pTween->PlayTween(startAlpha, 0.f, playtime,
				[handle](_float value)
				{
					if (auto* pTarget = GetSafeUI(handle))
						pTarget->SetAlpha(value);
				}, nullptr, EEaseType::EaseOutQuad, delay);
		}
	}

	// SpellMeter는 알파 전환과 별개로 원래 ScaleRatio를 기억한 뒤 0까지 축소한다.
	if (const auto* pLayer = CGameInstance::Get().GetGameObjectLayer("Layer_UI"))
	{
		for (const CHandle handle : *pLayer)
		{
			auto* pSpellMeter = E::CGameInstance::Get().GetGameObjectByHandleT<CSpellMeter>(handle);
			if (!pSpellMeter || !pSpellMeter->GetActive() || !pSpellMeter->GetVisible() ||
				pSpellMeter->GetResolvedRenderGroup() != RENDERGROUP::UI)
				continue;

			const _float startScale = pSpellMeter->GetScaleRatio();
			m_SpellMeterRestoreScale.try_emplace(handle, startScale);
			if (auto* pTween = pSpellMeter->GetTweenCom())
			{
				pTween->PlayTween(startScale, 0.f, playtime,
					[handle](_float value)
					{
						if (auto* pTarget = E::CGameInstance::Get().GetGameObjectByHandleT<CSpellMeter>(handle))
						{
							pTarget->SetScaleRatio(value);
							pTarget->CalcUICoord();
						}
					}, nullptr, EEaseType::EaseOutQuad, delay);
			}
		}
	}
}

void UIManager::PlayFadeInAll2DUI(float delay, float playtime)
{
	// FadeOut 시점에 실제로 숨긴 UI만 복원한다. 이후 생성된 선택창이나
	// 미니게임 UI에는 복원 Tween을 걸지 않는다.
	for (const auto& [handle, targetAlpha] : m_2DUIRestoreAlpha)
	{
		auto* pUI = GetSafeUI(handle);
		if (!pUI || !pUI->GetActive() || !pUI->GetVisible() ||
			pUI->GetResolvedRenderGroup() != RENDERGROUP::UI ||
			pUI->GetUIInfo().UIType == ETOUI(UI_TYPE::CURSOR))
			continue;

		const auto lockIt = m_2DUIRestoreInputLock.find(handle);
		const _bool restoreInputLock = lockIt != m_2DUIRestoreInputLock.end() ?
			lockIt->second : false;
		if (m_bQuestFadeSuppressed && m_hQuestRoot && handle == *m_hQuestRoot)
		{
			pUI->SetAlpha(0.f);
			pUI->SetInputLcok(restoreInputLock);
			continue;
		}

		if (auto* pTween = pUI->GetTweenCom())
		{
			const _float startAlpha = pUI->GetAlpha();
			pTween->PlayTween(startAlpha, targetAlpha, playtime,
				[handle](_float value)
				{
					if (auto* pTarget = GetSafeUI(handle))
						pTarget->SetAlpha(value);
				}, [handle, restoreInputLock]()
				{
					if (auto* pTarget = GetSafeUI(handle))
						pTarget->SetInputLcok(restoreInputLock);
				}, EEaseType::EaseOutQuad, delay);
		}
		else
		{
			pUI->SetAlpha(targetAlpha);
			pUI->SetInputLcok(restoreInputLock);
		}
	}
	m_2DUIRestoreAlpha.clear();
	m_2DUIRestoreInputLock.clear();

	// FadeOut에서 저장한 SpellMeter의 고유 ScaleRatio로 천천히 복구한다.
	for (auto it = m_SpellMeterRestoreScale.begin(); it != m_SpellMeterRestoreScale.end();)
	{
		const CHandle handle = it->first;
		const _float targetScale = it->second;
		auto* pSpellMeter = E::CGameInstance::Get().GetGameObjectByHandleT<CSpellMeter>(handle);
		if (!pSpellMeter || pSpellMeter->GetResolvedRenderGroup() != RENDERGROUP::UI)
		{
			it = m_SpellMeterRestoreScale.erase(it);
			continue;
		}

		if (auto* pTween = pSpellMeter->GetTweenCom())
		{
			const _float startScale = pSpellMeter->GetScaleRatio();
			pTween->PlayTween(startScale, targetScale, playtime,
				[handle](_float value)
				{
					if (auto* pTarget = E::CGameInstance::Get().GetGameObjectByHandleT<CSpellMeter>(handle))
					{
						pTarget->SetScaleRatio(value);
						pTarget->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, delay);
		}
		++it;
	}
	m_SpellMeterRestoreScale.clear();
}

void UIManager::PlayFadeInChange(CHandle pHandle, LEVEL level, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(0.f, 1.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, [pHandle, level]() {
			Engine::CGameInstance::Get().ChangeLevel(
				CLevelLoading::Create(E::CGameInstance::Get().GetGraphicDevice(), E::CGameInstance::Get().GetGraphicDeviceContext(), level));
			}, EEaseType::EaseOutQuad, delay);
}

