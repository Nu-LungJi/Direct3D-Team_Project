#include "pch.h"
#include "UIController.h"
#include "UIManager.h"
#include "HPBar.h"
#include "SpellMeter.h"
#include "TextBox.h"
#include "Button.h"
#include "Cursor.h"
#include "Monster.h"
#include "EnderDragon.h"
#include "BossTMB.h"
#include "Spider.h"
#include "MiniMap.h"
#include "ClientEvents.h"
#include "SpellMiniGame.h"
#include "Player.h"

NS_USING(Client)


namespace
{
	constexpr std::array<SPELL_TYPE, 20> SPELL_BUTTON_TYPES = {
		SPELL_TYPE::ARRESTOMOMENTUM, SPELL_TYPE::GLACIUS,
		SPELL_TYPE::LEVIOSO, SPELL_TYPE::TRANSFORMATION,
		SPELL_TYPE::ASSIO, SPELL_TYPE::DEPULSO, SPELL_TYPE::DESENDO,
		SPELL_TYPE::FLIPENDO, SPELL_TYPE::CONFRINGO, SPELL_TYPE::DIFFINDO,
		SPELL_TYPE::EXPELLIARMUS, SPELL_TYPE::BOMBARDA, SPELL_TYPE::INCENDIO,
		SPELL_TYPE::DISILLUSIONMENT, SPELL_TYPE::LUMOS, SPELL_TYPE::REPARO,
		SPELL_TYPE::WINGARDIUM, SPELL_TYPE::AVADAKEDAVRA,
		SPELL_TYPE::CRUCIO, SPELL_TYPE::IMPERIO
	};
}

CUIController::CUIController()
{
}

CUIController::~CUIController()
{
}

HRESULT CUIController::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CUIController::Initialize(void* pArg)
{
	auto		pDesc = static_cast<GAMEOBJECT_DESC*>(pArg);

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	CreatePlayScreen();
	BindMiniMap();
	SubscribeQuestUIEvents();
	ActivePlayScreen = true;
	
	{
		auto clientSize = CGameInstance::Get().GetClientScreenSize();
		std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

		CCursor::UIOBJECT_DESC Desc{};

		Desc.sObjectTag = "Cursor";
		Desc.Name = "Cursor";
		Desc.fSizeX = 64.f;
		Desc.fSizeY = 64.f;
		Desc.fX = clientSize.x * 0.5f;
		Desc.fY = clientSize.y * 0.5f;
		Desc.fAlpha = 1.f;
		Desc.UIType = ETOUI(UI_TYPE::CURSOR);
		Desc.ResWeight = 1000;

		m_Cursor = E::CGameInstance::Get().AddGameObjectToLayer(currentLevel, "Prototype_GameObject_Cursor", "Layer_UI", &Desc);

		/****페이드인*****/
		//PlayFadeOutDelete(GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front(), 1.f, 2.f);
	}

	return S_OK;
}

void CUIController::PriorityUpdate(E::_float fTimeDelta)
{	
}

void CUIController::Update(E::_float fTimeDelta)
{
	BindMiniMap();
	ApplyPendingQuestUIGroups();
	UpdateRookwoodQuestProgression();
	UpdateRookwoodSecondBattleCompletion();
	UpdateRookwoodThirdBattleCompletion();
	UpdateRookwoodBridgeProgression();
	UpdateRookwoodPortalProgression();

	// 전체 레이스 미니게임 흐름 확인용 디버그 입력.
	if (E::CGameInstance::Get().KeyDown(DIK_F3))
	{
		GET_SINGLE(UIManager)->StartRaceMiniGame();
	}

	// World-space RTT wand-shop debug entry.
	if (E::CGameInstance::Get().KeyDown(DIK_F4))
	{
		CPlayer* player = nullptr;
		CHandle playerHandle{};
		if (const auto* playerLayer = E::CGameInstance::Get().GetGameObjectLayer("03_Player"))
		{
			for (const CHandle handle : *playerLayer)
			{
				player = E::CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(handle);
				if (player)
				{
					playerHandle = handle;
					break;
				}
			}
		}

		if (player)
		{
			GET_SINGLE(UIManager)->OpenWandShopWorld(
				playerHandle,
				{ 0.f, 1.6f, 3.f },
				{ 0.f, 0.f, 0.f });
		}
	}

	// 대화 선택 UI와 콜백 연결 확인용 디버그 입력.
	if (E::CGameInstance::Get().KeyDown(DIK_F5))
	{
		const CHandle controllerHandle = GetHandle();
		GET_SINGLE(UIManager)->PlayFadeOutAll2DUI(0.f, 0.25f);
		GET_SINGLE(UIManager)->CreateChoiceUI(
			{
				"스펠 미니게임 1 시작",
				"레이스 미니게임 시작"
			},
			[controllerHandle](size_t choiceIndex)
			{
				GET_SINGLE(UIManager)->PlayFadeInAll2DUI(0.2f, 0.3f);

				if (choiceIndex == 0u)
				{
					if (auto* controller = E::CGameInstance::Get().
						GetGameObjectByHandleT<CUIController>(controllerHandle))
					{
						controller->StartSpellMiniGame(false);
					}
				}
				else if (choiceIndex == 1u)
				{
					GET_SINGLE(UIManager)->StartRaceMiniGame();
				}
			});
	}

	if (m_hSpellMiniGame && !E::CGameInstance::Get().
		GetGameObjectByHandleT<CSpellMiniGame>(*m_hSpellMiniGame))
	{
		m_hSpellMiniGame = std::nullopt;
		FadeOutSpellMiniGameBackground();
		FadeInPotionCountAfterSpellMiniGame();
		FadeInQuestAfterSpellMiniGame();
		E::CGameInstance::Get().SetMouseFix(true);
		if (m_Cursor)
		{
			if (auto* cursor = SafeGetOBJ(*m_Cursor))
				cursor->SetAlpha(0.f);
		}
	}

	// Temporary test entries for the two spell mini-game layouts.
	if (E::CGameInstance::Get().KeyDown(DIK_F6))
	{
		if (m_hSpellMiniGame)
			StopSpellMiniGame();
		else
			StartSpellMiniGame(true);
	}

	if (E::CGameInstance::Get().KeyDown(DIK_F7))
	{
		if (m_hSpellMiniGame)
			StopSpellMiniGame();
		else
			StartSpellMiniGame(false);
	}

	if (!CursorCreate)
	{

		auto clientSize = CGameInstance::Get().GetClientScreenSize();
		std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();
		
		CCursor::UIOBJECT_DESC Desc{};
		
		Desc.sObjectTag = "Cursor";
		Desc.Name = "Cursor";
		Desc.fSizeX = 64.f;
		Desc.fSizeY = 64.f;
		Desc.fX = clientSize.x * 0.5f;
		Desc.fY = clientSize.y * 0.5f;
		Desc.fAlpha = 1.f;
		Desc.UIType = ETOUI(UI_TYPE::CURSOR);
		Desc.ResWeight = 1000;
		
		m_Cursor = E::CGameInstance::Get().AddGameObjectToLayer(currentLevel, "Prototype_GameObject_Cursor", "Layer_UI", &Desc);

		CursorCreate = true;

	}

	// ************** 플레이어 HP
	if (E::CGameInstance::Get().KeyDown(DIK_9))
	{
		AddHP(-200.f);
	}

	// ************** 플레이어 Finisher
	if (E::CGameInstance::Get().KeyDown(DIK_8))
	{
		AddFinisher(10.f);
	}
	if (E::CGameInstance::Get().KeyDown(DIK_7))
	{
		AddFinisher(-10.f);
	}

	// ************** 스펠슬롯
	if (E::CGameInstance::Get().KeyDown(DIK_1))
	{
		UseSpell(1);
	}
	else if (E::CGameInstance::Get().KeyDown(DIK_2))
	{
		UseSpell(2);
	}
	else if (E::CGameInstance::Get().KeyDown(DIK_3))
	{
		UseSpell(3);
	}
	else if (E::CGameInstance::Get().KeyDown(DIK_4))
	{
		UseSpell(4);
	}

	// ************** 포션
	if (E::CGameInstance::Get().KeyDown(DIK_NEXT))
	{
		UsePotion();
		E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/Potion.wav", SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::UI,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});
	}
	// ************** 스펠슬롯
	if (E::CGameInstance::Get().KeyDown(DIK_B))
	{
		ActiveShortCutSlot ? ActiveShortCutSlot = false : ActiveShortCutSlot = true;
		if (ActiveShortCutSlot)
		{
			CreateSpellType();
			SafeGetOBJ(m_PotionCount)->SetActive(false);
		}
		else
		{
			DeleteSpellType();
			SafeGetOBJ(m_PotionCount)->SetActive(true);
		}
			
	}

	// 몬스터 HP 감송
	if (E::CGameInstance::Get().KeyDown(DIK_6))
		AddMonsterHP(-30.f);

	// 죽는 화면
	//if (E::CGameInstance::Get().KeyDown(DIK_0) && !m_isCreateDeathScene)
	//{
	//	m_isCreateDeathScene = true;
	//	CreateDeathScene();
	//}
	

	/****************필수********************/
	if (m_bMonsterHP)
	{
		CreateMonsterHP();
		m_bMonsterHP = false;
	}

	UpdateMonsterHP();
}

void CUIController::LateUpdate(E::_float fTimeDelta)
{
}

HRESULT CUIController::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

_bool CUIController::StartSpellMiniGame(_bool secondGame)
{
	if (m_hSpellMiniGame && E::CGameInstance::Get().
		GetGameObjectByHandleT<CSpellMiniGame>(*m_hSpellMiniGame))
	{
		return false;
	}

	m_hSpellMiniGame = std::nullopt;
	const std::string currentLevel = _string("LEVEL_") +
		MagicEnumToStringView(static_cast<LEVEL>(
			E::CGameInstance::Get().GetCurrentLevelID())).data();

	CSpellMiniGame::DESC desc{};
	desc.sObjectTag = secondGame
		? "SpellMiniGame_Flipendo"
		: "SpellMiniGame_Incendio";
	desc.Mode = secondGame
		? CSpellMiniGame::MODE::FLIPENDO
		: CSpellMiniGame::MODE::INCENDIO;
	auto handle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_SpellMiniGame",
		"Layer_UI",
		&desc);
	if (!handle)
		return false;

	m_hSpellMiniGame = *handle;
	E::CGameInstance::Get().GetSoundManager()->Play2D(
		"./Resources/SampleClient/Sound/UI/Book.wav",
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::UI,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});
	const std::vector<CHandle> backgroundRoots =
		GET_SINGLE(UIManager)->LoadPrefab("BlackBG250");
	if (!backgroundRoots.empty())
	{
		m_hSpellMiniGameBackground = backgroundRoots.front();
		if (auto* background = SafeGetOBJ(*m_hSpellMiniGameBackground))
		{
			const _float targetAlpha = background->GetAlpha();
			background->SetAlpha(0.f);
			background->SetInputLcok(true);
			background->CalcUICoord();
			background->GetTweenCom()->PlayTween(
				0.f,
				targetAlpha,
				0.3f,
				[background](_float currentAlpha)
				{
					background->SetAlpha(currentAlpha);
				},
				nullptr,
				EEaseType::EaseOutQuad);
		}
		else
		{
			m_hSpellMiniGameBackground = std::nullopt;
		}
	}
	FadeOutPotionCountForSpellMiniGame();
	FadeOutQuestForSpellMiniGame();
	E::CGameInstance::Get().SetMouseFix(false);
	if (m_Cursor)
	{
		if (auto* cursor = SafeGetOBJ(*m_Cursor))
			cursor->SetAlpha(1.f);
	}
	return true;
}

void CUIController::StopSpellMiniGame()
{
	if (m_hSpellMiniGame)
	{
		if (auto* miniGame = E::CGameInstance::Get().
			GetGameObjectByHandleT<CSpellMiniGame>(*m_hSpellMiniGame))
		{
			miniGame->SetPendingDestroyCascade();
		}
	}

	m_hSpellMiniGame = std::nullopt;
	FadeOutSpellMiniGameBackground();
	FadeInPotionCountAfterSpellMiniGame();
	FadeInQuestAfterSpellMiniGame();
	E::CGameInstance::Get().SetMouseFix(true);
	if (m_Cursor)
	{
		if (auto* cursor = SafeGetOBJ(*m_Cursor))
			cursor->SetAlpha(0.f);
	}
}

void CUIController::FadeOutSpellMiniGameBackground()
{
	if (!m_hSpellMiniGameBackground)
		return;

	const CHandle backgroundHandle = *m_hSpellMiniGameBackground;
	m_hSpellMiniGameBackground = std::nullopt;
	if (auto* background = SafeGetOBJ(backgroundHandle))
	{
		const _float startAlpha = background->GetAlpha();
		background->SetInputLcok(true);
		background->GetTweenCom()->PlayTween(
			startAlpha,
			0.f,
			0.5f,
			[backgroundHandle](_float currentAlpha)
			{
				if (auto* background = GetSafeUI(backgroundHandle))
					background->SetAlpha(currentAlpha);
			},
			[backgroundHandle]()
			{
				if (GetSafeUI(backgroundHandle))
					GET_SINGLE(UIManager)->DeleteUIRecursive(backgroundHandle);
			},
			EEaseType::EaseOutQuad);
	}
}

void CUIController::FadeOutPotionCountForSpellMiniGame()
{
	auto* potionCount = SafeGetOBJ(m_PotionCount);
	if (!potionCount)
		return;

	potionCount->GetTweenCom()->ClearTweens();
	const _float startAlpha = potionCount->GetAlpha();
	potionCount->GetTweenCom()->PlayTween(
		startAlpha,
		0.f,
		0.3f,
		[potionCount](_float currentAlpha)
		{
			potionCount->SetAlpha(currentAlpha);
		},
		nullptr,
		EEaseType::EaseOutQuad);
}

void CUIController::FadeInPotionCountAfterSpellMiniGame()
{
	auto* potionCount = SafeGetOBJ(m_PotionCount);
	if (!potionCount)
		return;

	potionCount->GetTweenCom()->ClearTweens();
	const _float startAlpha = potionCount->GetAlpha();
	potionCount->GetTweenCom()->PlayTween(
		startAlpha,
		1.f,
		0.5f,
		[potionCount](_float currentAlpha)
		{
			potionCount->SetAlpha(currentAlpha);
		},
		nullptr,
		EEaseType::EaseOutQuad);
}

void CUIController::FadeOutQuestForSpellMiniGame()
{
	GET_SINGLE(UIManager)->FadeOutQuest(0.3f);

	if (!m_hQuestRoot)
		return;

	const CHandle questHandle = *m_hQuestRoot;
	auto* questRoot = SafeGetOBJ(questHandle);
	if (!questRoot || !questRoot->GetTweenCom())
		return;

	questRoot->GetTweenCom()->ClearTweens();
	const _float startAlpha = questRoot->GetAlpha();
	questRoot->GetTweenCom()->PlayTween(
		startAlpha, 0.f, 0.3f,
		[questHandle](_float currentAlpha)
		{
			if (auto* quest = GetSafeUI(questHandle))
				quest->SetAlpha(currentAlpha);
		},
		nullptr,
		EEaseType::EaseOutQuad);
}

void CUIController::FadeInQuestAfterSpellMiniGame()
{
	GET_SINGLE(UIManager)->FadeInQuest(0.5f);

	if (!m_hQuestRoot)
		return;

	const CHandle questHandle = *m_hQuestRoot;
	auto* questRoot = SafeGetOBJ(questHandle);
	if (!questRoot || !questRoot->GetTweenCom())
		return;

	questRoot->GetTweenCom()->ClearTweens();
	const _float startAlpha = questRoot->GetAlpha();
	questRoot->GetTweenCom()->PlayTween(
		startAlpha, 1.f, 0.5f,
		[questHandle](_float currentAlpha)
		{
			if (auto* quest = GetSafeUI(questHandle))
				quest->SetAlpha(currentAlpha);
		},
		nullptr,
		EEaseType::EaseOutQuad);
}

void CUIController::CreatePlayScreen()
{
	/*******플레이어 체력*******/
	m_PlayerHP = GET_SINGLE(UIManager)->LoadPrefab("PlayerHP").front();
	static_cast<CHPBar*>(SafeGetOBJ(m_PlayerHP))->SetMaxFill(500);
	static_cast<CHPBar*>(SafeGetOBJ(m_PlayerHP))->SetCurrentFill(500);
	/*******피니셔*******/
	m_Finisher[0] = GET_SINGLE(UIManager)->LoadPrefab("Finisher1").front();
	m_Finisher[1] = GET_SINGLE(UIManager)->LoadPrefab("Finisher2").front();
	m_Finisher[2] = GET_SINGLE(UIManager)->LoadPrefab("Finisher3").front();
	m_FinisherIcon = GET_SINGLE(UIManager)->LoadPrefab("FinisherIcon").front();
	for (auto pHandle : m_Finisher)
	{
		static_cast<CHPBar*>(SafeGetOBJ(pHandle))->SetAmount(0.f);
	}
	SetFinisher(0.f);

	/*******스펠슬롯*******/
	m_SpellSlot[0] = GET_SINGLE(UIManager)->LoadPrefab("SpellSlot1").front();
	m_SpellSlot[1] = GET_SINGLE(UIManager)->LoadPrefab("SpellSlot2").front();
	m_SpellSlot[2] = GET_SINGLE(UIManager)->LoadPrefab("SpellSlot3").front();
	m_SpellSlot[3] = GET_SINGLE(UIManager)->LoadPrefab("SpellSlot4").front();

	for (uint32_t i = 0; i < 4u; ++i)
	{
		auto* spellMeter = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[i]));
		spellMeter->SetSpellType(GET_SINGLE(UIManager)->GetSavedSpellSlot(i + 1u));
		spellMeter->SetResTagDirtyFlag(true);
	}

	/*******포션 개수*******/
	m_PotionCount = GET_SINGLE(UIManager)->LoadPrefab("PotionCount").front();

	/*******정적 유아이*******/
	m_PlaySceneStatic = GET_SINGLE(UIManager)->LoadPrefab("StaticPlayScreen");
}

_bool CUIController::SetQuestUIGroupActive(
	QUEST_UI_GROUP group, _bool active,
	const std::string& questText,
	_bool updateMinimap,
	_bool updateQuestWidget)
{
	const size_t index = static_cast<size_t>(group);
	if (group == QUEST_UI_GROUP::NONE ||
		group == QUEST_UI_GROUP::END ||
		index >= QUEST_UI_GROUP_COUNT)
		return false;

	if (updateMinimap && m_QuestUIGroupStates[index] != active)
	{
		m_QuestUIGroupStates[index] = active;
		m_QuestUIGroupDirty[index] = true;
	}
	if (!questText.empty())
		m_QuestUIGroupTexts[index] = questText;

	if (updateMinimap)
		ApplyPendingQuestUIGroups();
	if (updateQuestWidget)
		RefreshQuestWidget(group, active);
	return true;
}

void CUIController::BindMiniMap()
{
	if (m_hMiniMap && E::CGameInstance::Get().
		GetGameObjectByHandleT<CMiniMap>(*m_hMiniMap))
		return;

	m_hMiniMap = std::nullopt;
	std::vector<CHandle> pendingHandles = m_PlaySceneStatic;
	for (size_t i = 0; i < pendingHandles.size(); ++i)
	{
		const CHandle handle = pendingHandles[i];
		if (E::CGameInstance::Get().
			GetGameObjectByHandleT<CMiniMap>(handle))
		{
			m_hMiniMap = handle;
			for (size_t groupIndex = 1;
				groupIndex < QUEST_UI_GROUP_COUNT; ++groupIndex)
			{
				m_QuestUIGroupDirty[groupIndex] = true;
			}
			return;
		}

		auto* uiObject = E::CGameInstance::Get().
			GetGameObjectByHandleT<CUIObject>(handle);
		if (!uiObject)
			continue;

		const auto& children = uiObject->GetChildren();
		pendingHandles.insert(
			pendingHandles.end(), children.begin(), children.end());
	}
}

void CUIController::SubscribeQuestUIEvents()
{
	if (m_iQuestUIListenerID != 0)
		return;

	m_iQuestUIListenerID = E::CGameInstance::Get().
		EventSubscribe<FQuestUIGroupChanged>(
			GetHandle(),
			[this](const FQuestUIGroupChanged& event)
			{
				SetQuestUIGroupActive(
					event.Group, event.Active, event.QuestText,
					event.UpdateMinimap, event.UpdateQuestWidget);
			});
}

std::string CUIController::GetQuestDisplayText(QUEST_UI_GROUP group) const
{
	const size_t index = static_cast<size_t>(group);
	if (index < m_QuestUIGroupTexts.size() &&
		!m_QuestUIGroupTexts[index].empty())
	{
		return m_QuestUIGroupTexts[index];
	}

	switch (group)
	{
	case QUEST_UI_GROUP::ROOKWOOD_TRIAL_01:
		return "퍼시벌 랙햄의 시험을 완료하기";
	case QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_TRIAL_02:
		return "퍼시벌 랙햄의 시험을 완료하기";
	case QUEST_UI_GROUP::ROOKWOOD_TRIAL_02:
		return "두 번째 전투 구역으로 이동하기";
	case QUEST_UI_GROUP::ROOKWOOD_TRIAL_03:
		return "세 번째 전투 구역으로 이동하기";
	case QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_BRIDGE:
		return "퍼시벌 랙햄의 시험을 완료하기";
	case QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_PORTAL:
		return "퍼시벌 랙햄의 시험을 완료하기";
	default:
		return {};
	}
}

void CUIController::RefreshQuestWidget(
	QUEST_UI_GROUP changedGroup, _bool active)
{
	if (active)
	{
		ShowQuestWidget(changedGroup);
		return;
	}

	if (m_eDisplayedQuestGroup != changedGroup)
		return;

	for (size_t index = 1; index < QUEST_UI_GROUP_COUNT; ++index)
	{
		if (m_QuestUIGroupStates[index])
		{
			ShowQuestWidget(static_cast<QUEST_UI_GROUP>(index));
			return;
		}
	}

	HideQuestWidget();
}

void CUIController::ShowQuestWidget(QUEST_UI_GROUP group)
{
	const std::string displayText = GetQuestDisplayText(group);
	if (displayText.empty())
		return;

	auto* root = m_hQuestRoot ? GetSafeUI(*m_hQuestRoot) : nullptr;
	auto* text = m_hQuestText ? E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextBox>(*m_hQuestText) : nullptr;

	if (!root || !text)
	{
		m_hQuestRoot = std::nullopt;
		m_hQuestText = std::nullopt;
		const auto questObjects = GET_SINGLE(UIManager)->LoadPrefab("Quest");
		for (const CHandle handle : questObjects)
		{
			auto* ui = GetSafeUI(handle);
			if (!ui)
				continue;
			if (std::string_view(ui->GetName()) == "QuestFrame")
				m_hQuestRoot = handle;
			else if (std::string_view(ui->GetName()) == "QuestText")
				m_hQuestText = handle;
		}

		root = m_hQuestRoot ? GetSafeUI(*m_hQuestRoot) : nullptr;
		text = m_hQuestText ? E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextBox>(*m_hQuestText) : nullptr;
		if (!root || !text)
		{
			HideQuestWidget();
			return;
		}

		text->SetwText(StringToWUTF8(displayText));
		m_eDisplayedQuestGroup = group;
		root->SetAlpha(0.f);

		// A freshly loaded TextureUI clears its tweens when its first APPEAR
		// state is processed. Start the fade from the APPEAR callback so the
		// initial quest does not remain at alpha zero.
		const CHandle questRootHandle = *m_hQuestRoot;
		root->Appear = [questRootHandle](CUIObject*)
		{
			if (auto* questRoot = GetSafeUI(questRootHandle))
			{
				questRoot->SetAlpha(0.f);
				GET_SINGLE(UIManager)->PlayFadeIn(
					questRootHandle, 0.f, 0.3f);
			}
		};
		return;
	}

	if (m_eDisplayedQuestGroup == group &&
		text->GetwText() == StringToWUTF8(displayText))
		return;

	auto* tween = text->GetTweenCom();
	if (!tween)
	{
		text->SetwText(StringToWUTF8(displayText));
		m_eDisplayedQuestGroup = group;
		return;
	}

	tween->ClearTweens();
	const CHandle textHandle = *m_hQuestText;
	const _float baseLocalX = text->GetUIInfo().LocalX;
	const _float baseLocalY = text->GetUIInfo().LocalY;
	constexpr _float shift = 18.f;
	constexpr _float outDuration = 0.2f;
	constexpr _float inDuration = 0.25f;
	const std::wstring nextText = StringToWUTF8(displayText);
	m_eDisplayedQuestGroup = group;

	tween->PlayTween(
		text->GetAlphaRatio(), 0.f, outDuration,
		[textHandle](_float value)
		{
			if (auto* ui = GetSafeUI(textHandle))
				ui->SetAlphaRatio(value);
		},
		[textHandle, nextText, baseLocalX, baseLocalY]()
		{
			if (auto* textBox = E::CGameInstance::Get().
				GetGameObjectByHandleT<CTextBox>(textHandle))
			{
				textBox->SetwText(nextText);
				textBox->SetLocalPos({ baseLocalX - shift, baseLocalY });
				textBox->SetAlphaRatio(0.f);
				textBox->CalcUICoord();
			}
		}, EEaseType::EaseOutQuad);
	tween->PlayTween(
		baseLocalX, baseLocalX - shift, outDuration,
		[textHandle, baseLocalY](_float value)
		{
			if (auto* ui = GetSafeUI(textHandle))
			{
				ui->SetLocalPos({ value, baseLocalY });
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
		baseLocalX - shift, baseLocalX, inDuration,
		[textHandle, baseLocalY](_float value)
		{
			if (auto* ui = GetSafeUI(textHandle))
			{
				ui->SetLocalPos({ value, baseLocalY });
				ui->CalcUICoord();
			}
		}, nullptr, EEaseType::EaseOutQuad, outDuration);
}

void CUIController::HideQuestWidget()
{
	if (m_hQuestRoot && GetSafeUI(*m_hQuestRoot))
		GET_SINGLE(UIManager)->PlayFadeOutDelete(*m_hQuestRoot, 0.f, 0.3f);
	m_hQuestRoot = std::nullopt;
	m_hQuestText = std::nullopt;
	m_eDisplayedQuestGroup = QUEST_UI_GROUP::NONE;
}

void CUIController::ApplyPendingQuestUIGroups()
{
	if (!m_hMiniMap)
		return;

	auto* miniMap = E::CGameInstance::Get().
		GetGameObjectByHandleT<CMiniMap>(*m_hMiniMap);
	if (!miniMap)
	{
		m_hMiniMap = std::nullopt;
		return;
	}

	for (size_t index = 1; index < QUEST_UI_GROUP_COUNT; ++index)
	{
		if (!m_QuestUIGroupDirty[index])
			continue;

		const auto group = static_cast<QUEST_UI_GROUP>(index);
		if (miniMap->SetContentGroupActive(
			group, m_QuestUIGroupStates[index]))
		{
			m_QuestUIGroupDirty[index] = false;
		}
	}
}

void CUIController::UpdateRookwoodQuestProgression()
{
	if (m_bRookwoodSecondApproachReached ||
		E::CGameInstance::Get().GetCurrentLevelID() !=
			ETOUI(LEVEL::CHARLES_ROOKWOOD))
	{
		return;
	}

	const size_t moveGroupIndex = static_cast<size_t>(
		QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_TRIAL_02);
	if (moveGroupIndex >= m_QuestUIGroupStates.size() ||
		!m_QuestUIGroupStates[moveGroupIndex])
	{
		return;
	}

	CPlayer* player = nullptr;
	if (const auto* playerLayer =
		E::CGameInstance::Get().GetGameObjectLayer("03_Player"))
	{
		for (const CHandle handle : *playerLayer)
		{
			player = E::CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer>(handle);
			if (player)
				break;
		}
	}
	if (!player)
		return;

	constexpr _float3 approachPosition{
		-253.683f, -223.682f, -54.548f
	};
	constexpr _float arrivalRadius = 12.f;
	const _float3 playerPosition = player->GetTransform().GetPosition();
	const _float deltaX = playerPosition.x - approachPosition.x;
	const _float deltaZ = playerPosition.z - approachPosition.z;
	if (deltaX * deltaX + deltaZ * deltaZ >
		arrivalRadius * arrivalRadius)
	{
		return;
	}

	m_bRookwoodSecondApproachReached = true;
	SetQuestUIGroupActive(
		QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_TRIAL_02,
		false, {}, true, false);
	SetQuestUIGroupActive(
		QUEST_UI_GROUP::ROOKWOOD_TRIAL_02,
		true,
		"퍼시벌 랙햄의 시험을 완료하기",
		true, false);
}

void CUIController::UpdateRookwoodSecondBattleCompletion()
{
	if (m_bRookwoodSecondBattleCompleted ||
		E::CGameInstance::Get().GetCurrentLevelID() !=
			ETOUI(LEVEL::CHARLES_ROOKWOOD))
	{
		return;
	}

	const size_t groupIndex = static_cast<size_t>(
		QUEST_UI_GROUP::ROOKWOOD_TRIAL_02);
	if (groupIndex >= m_QuestUIGroupStates.size() ||
		!m_QuestUIGroupStates[groupIndex] ||
		m_QuestUIGroupTexts[groupIndex] !=
			"경비병들을 쓰러트리기")
	{
		return;
	}

	// 전투 트리거가 몬스터를 레이어에 완전히 추가한 다음 프레임에
	// 실제 두 번째 구역 몬스터 두 마리의 핸들을 확보한다.
	if (m_RookwoodSecondBattleMonsters.empty())
	{
		const auto* monsterLayer = E::CGameInstance::Get().
			GetGameObjectLayer("02_TmbGurdian");
		if (!monsterLayer)
			return;

		constexpr _float3 battleCenter{
			-252.469f, -224.784f, -109.236f
		};
		constexpr _float collectRadius = 40.f;
		for (const CHandle handle : *monsterLayer)
		{
			auto* monster = E::CGameInstance::Get().
				GetGameObjectByHandleT<CMonster>(handle);
			if (!monster || monster->Get_CurrentHp() <= 0)
				continue;

			const _float3 position = monster->GetTransform().GetPosition();
			const _float deltaX = position.x - battleCenter.x;
			const _float deltaZ = position.z - battleCenter.z;
			if (deltaX * deltaX + deltaZ * deltaZ >
				collectRadius * collectRadius)
			{
				continue;
			}

			m_RookwoodSecondBattleMonsters.push_back(handle);
		}

		if (m_RookwoodSecondBattleMonsters.size() < 2)
		{
			m_RookwoodSecondBattleMonsters.clear();
			return;
		}
	}

	const _bool allMonstersDefeated = std::all_of(
		m_RookwoodSecondBattleMonsters.begin(),
		m_RookwoodSecondBattleMonsters.end(),
		[](CHandle handle)
		{
			auto* monster = E::CGameInstance::Get().
				GetGameObjectByHandleT<CMonster>(handle);
			if (!monster || monster->GetPendingDestroy() ||
				monster->Get_CurrentHp() <= 0)
			{
				return true;
			}

			// 경비병은 낭떠러지로 떨어져도 HP/DEAD 플래그가 바뀌지 않고
			// 중력 상태로 계속 남는다. 두 번째 전투 바닥(-230 부근)보다
			// 충분히 아래로 떨어졌다면 전투 목표에서는 처치로 판정한다.
			constexpr _float fallDefeatY = -245.f;
			return monster->GetTransform().GetPosition().y < fallDefeatY;
		});
	if (!allMonstersDefeated)
		return;

	m_bRookwoodSecondBattleCompleted = true;
	GET_SINGLE(UIManager)->CreateOrChangeQuest(
		"퍼시벌 랙햄의 시험을 완료하기");
	SetQuestUIGroupActive(
		QUEST_UI_GROUP::ROOKWOOD_TRIAL_02,
		false, {}, true, false);
	SetQuestUIGroupActive(
		QUEST_UI_GROUP::ROOKWOOD_TRIAL_03,
		true,
		"퍼시벌 랙햄의 시험을 완료하기",
		true, false);
}

void CUIController::UpdateRookwoodThirdBattleCompletion()
{
	if (m_bRookwoodThirdBattleCompleted ||
		E::CGameInstance::Get().GetCurrentLevelID() !=
			ETOUI(LEVEL::CHARLES_ROOKWOOD))
	{
		return;
	}

	const size_t groupIndex = static_cast<size_t>(
		QUEST_UI_GROUP::ROOKWOOD_TRIAL_03);
	if (groupIndex >= m_QuestUIGroupStates.size() ||
		!m_QuestUIGroupStates[groupIndex] ||
		m_QuestUIGroupTexts[groupIndex] !=
			"엘리트 경비병들을 쓰러트리기")
	{
		return;
	}

	// 세 번째 구역에는 레벨 진입 시 엘리트 두 마리가 미리 배치된다.
	// 타입 getter에 의존하지 않고 전투 중심에서 가까운 두 몬스터를
	// 추적해 일반 사망, 삭제, 낙사 모두 같은 완료 조건으로 처리한다.
	if (m_RookwoodThirdBattleMonsters.empty())
	{
		const auto* monsterLayer = E::CGameInstance::Get().
			GetGameObjectLayer("02_TmbGurdian");
		if (!monsterLayer)
			return;

		constexpr _float3 battleCenter{
			-254.361f, -223.280f, -209.996f
		};
		constexpr _float collectRadius = 45.f;
		std::vector<std::pair<_float, CHandle>> candidates{};
		for (const CHandle handle : *monsterLayer)
		{
			auto* monster = E::CGameInstance::Get().
				GetGameObjectByHandleT<CMonster>(handle);
			if (!monster || monster->GetPendingDestroy() ||
				monster->Get_CurrentHp() <= 0)
			{
				continue;
			}

			const _float3 position = monster->GetTransform().GetPosition();
			const _float deltaX = position.x - battleCenter.x;
			const _float deltaZ = position.z - battleCenter.z;
			const _float distanceSq = deltaX * deltaX + deltaZ * deltaZ;
			if (distanceSq <= collectRadius * collectRadius)
				candidates.emplace_back(distanceSq, handle);
		}

		if (candidates.size() < 2)
			return;

		std::sort(candidates.begin(), candidates.end(),
			[](const auto& lhs, const auto& rhs)
			{
				return lhs.first < rhs.first;
			});
		m_RookwoodThirdBattleMonsters.push_back(candidates[0].second);
		m_RookwoodThirdBattleMonsters.push_back(candidates[1].second);
	}

	const _bool allMonstersDefeated = std::all_of(
		m_RookwoodThirdBattleMonsters.begin(),
		m_RookwoodThirdBattleMonsters.end(),
		[](CHandle handle)
		{
			auto* monster = E::CGameInstance::Get().
				GetGameObjectByHandleT<CMonster>(handle);
			if (!monster || monster->GetPendingDestroy() ||
				monster->Get_CurrentHp() <= 0)
			{
				return true;
			}

			constexpr _float fallDefeatY = -245.f;
			return monster->GetTransform().GetPosition().y < fallDefeatY;
		});
	if (!allMonstersDefeated)
		return;

	m_bRookwoodThirdBattleCompleted = true;
	GET_SINGLE(UIManager)->CreateOrChangeQuest(
		"퍼시벌 랙햄의 시험을 완료하기");
	SetQuestUIGroupActive(
		QUEST_UI_GROUP::ROOKWOOD_TRIAL_03,
		false, {}, true, false);
	SetQuestUIGroupActive(
		QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_BRIDGE,
		true,
		"퍼시벌 랙햄의 시험을 완료하기",
		true, false);
}

void CUIController::UpdateRookwoodBridgeProgression()
{
	if (m_bRookwoodBridgeApproachReached ||
		E::CGameInstance::Get().GetCurrentLevelID() !=
			ETOUI(LEVEL::CHARLES_ROOKWOOD))
	{
		return;
	}

	const size_t groupIndex = static_cast<size_t>(
		QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_BRIDGE);
	if (groupIndex >= m_QuestUIGroupStates.size() ||
		!m_QuestUIGroupStates[groupIndex])
	{
		return;
	}

	CPlayer* player = nullptr;
	if (const auto* playerLayer =
		E::CGameInstance::Get().GetGameObjectLayer("03_Player"))
	{
		for (const CHandle handle : *playerLayer)
		{
			player = E::CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer>(handle);
			if (player)
				break;
		}
	}
	if (!player)
		return;

	constexpr _float3 bridgePosition{
		-252.617f, -239.471f, -378.125f
	};
	constexpr _float arrivalRadius = 12.f;
	const _float3 playerPosition = player->GetTransform().GetPosition();
	const _float deltaX = playerPosition.x - bridgePosition.x;
	const _float deltaZ = playerPosition.z - bridgePosition.z;
	if (deltaX * deltaX + deltaZ * deltaZ >
		arrivalRadius * arrivalRadius)
	{
		return;
	}

	m_bRookwoodBridgeApproachReached = true;
	GET_SINGLE(UIManager)->CreateOrChangeQuest("다리 복구하기");
	SetQuestUIGroupActive(
		QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_BRIDGE,
		true, "다리 복구하기", false, false);
}

void CUIController::UpdateRookwoodPortalProgression()
{
	if (m_bRookwoodPortalApproachReached ||
		E::CGameInstance::Get().GetCurrentLevelID() !=
			ETOUI(LEVEL::CHARLES_ROOKWOOD))
	{
		return;
	}

	const size_t groupIndex = static_cast<size_t>(
		QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_PORTAL);
	if (groupIndex >= m_QuestUIGroupStates.size() ||
		!m_QuestUIGroupStates[groupIndex])
	{
		return;
	}

	CPlayer* player = nullptr;
	if (const auto* playerLayer =
		E::CGameInstance::Get().GetGameObjectLayer("03_Player"))
	{
		for (const CHandle handle : *playerLayer)
		{
			player = E::CGameInstance::Get().
				GetGameObjectByHandleT<CPlayer>(handle);
			if (player)
				break;
		}
	}
	if (!player)
		return;

	constexpr _float3 portalPosition{
		-253.258f, -236.414f, -582.386f
	};
	constexpr _float arrivalRadius = 12.f;
	const _float3 playerPosition = player->GetTransform().GetPosition();
	const _float deltaX = playerPosition.x - portalPosition.x;
	const _float deltaZ = playerPosition.z - portalPosition.z;
	if (deltaX * deltaX + deltaZ * deltaZ >
		arrivalRadius * arrivalRadius)
	{
		return;
	}

	m_bRookwoodPortalApproachReached = true;
	GET_SINGLE(UIManager)->CreateOrChangeQuest("미지의 포탈에 빠지기");
	SetQuestUIGroupActive(
		QUEST_UI_GROUP::ROOKWOOD_MOVE_TO_PORTAL,
		true, "미지의 포탈에 빠지기", false, false);
}

void CUIController::CreateSpellType()
{
	GET_SINGLE(UIManager)->FadeOutQuest(0.3f);
	/********스펠슬롯**********/
	m_SpellBTNs = GET_SINGLE(UIManager)->LoadPrefab("OnlySpellBTN");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[0]))->SetResTag("TEX_UI_T_spellmeter_ArrestoMomentum_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[1]))->SetResTag("TEX_UI_T_spellmeter_Glacius_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[2]))->SetResTag("TEX_UI_T_spellmeter_Levioso_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[3]))->SetResTag("TEX_UI_T_spellmeter_TransformationOverlandOverlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[4]))->SetResTag("TEX_UI_T_spellmeter_Accio_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[5]))->SetResTag("TEX_UI_T_spellmeter_Depulso_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[6]))->SetResTag("TEX_UI_T_spellmeter_Descendo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[7]))->SetResTag("TEX_UI_T_spellmeter_Flipendo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[8]))->SetResTag("TEX_UI_T_spellmeter_Confringo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[9]))->SetResTag("TEX_UI_T_spellmeter_Diffindo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[10]))->SetResTag("TEX_UI_T_spellmeter_Expelliarmus_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[11]))->SetResTag("TEX_UI_T_spellmeter_Bombarda_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[12]))->SetResTag("TEX_UI_T_spellmeter_Incendio_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[13]))->SetResTag("TEX_UI_T_spellmeter_Disillusionment_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[14]))->SetResTag("TEX_UI_T_spellmeter_Lumos_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[15]))->SetResTag("TEX_UI_T_spellmeter_Reparo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[16]))->SetResTag("TEX_UI_T_spellmeter_WingardiumLeviosa_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[17]))->SetResTag("TEX_UI_T_spellmeter_AvadaKedavra_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[18]))->SetResTag("TEX_UI_T_spellmeter_Crucio_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[19]))->SetResTag("TEX_UI_T_spellmeter_Imperio_Overlay");

	/*********비디오 패스**********/
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[0]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_ArrestoMomentum.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[1]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Glacius.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[2]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Levioso.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[3]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Transformation.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[4]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Accio.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[5]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Depulso.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[6]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Descendo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[7]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Flipendo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[8]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Confringo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[9]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Diffindo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[10]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Expelliarmus.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[11]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Bombarda.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[12]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Incendio.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[13]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Disillusionment.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[14]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Lumos.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[15]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Reparo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[16]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_WingardiumLeviosa.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[17]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_AvadaKedavra.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[18]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Crucio.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[19]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Imperio.avi");

	/*********디스크립션 json 이름**********/
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[0]))->SetDescJsonname("AristoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[1]))->SetDescJsonname("GlaciusDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[2]))->SetDescJsonname("LeviosoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[3]))->SetDescJsonname("TransformDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[4]))->SetDescJsonname("AssioDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[5]))->SetDescJsonname("DepulsoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[6]))->SetDescJsonname("DescendoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[7]))->SetDescJsonname("FlipendoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[8]))->SetDescJsonname("ConfringoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[9]))->SetDescJsonname("DiffindoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[10]))->SetDescJsonname("ExpelliarmusDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[11]))->SetDescJsonname("BombardaDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[12]))->SetDescJsonname("IncendioDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[13]))->SetDescJsonname("DisillDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[14]))->SetDescJsonname("LumosDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[15]))->SetDescJsonname("ReparoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[16]))->SetDescJsonname("WingardiumDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[17]))->SetDescJsonname("AvadaKedavraDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[18]))->SetDescJsonname("CrucioDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[19]))->SetDescJsonname("ImperioDesc"); 
	ApplySpellLockStates();

	/********단축키슬롯**********/
	m_SpellShortCutKeySlot[0] = GET_SINGLE(UIManager)->LoadPrefab("ShortCut1").front();
	m_SpellShortCutKeySlot[1] = GET_SINGLE(UIManager)->LoadPrefab("ShortCut2").front();
	m_SpellShortCutKeySlot[2] = GET_SINGLE(UIManager)->LoadPrefab("ShortCut3").front();
	m_SpellShortCutKeySlot[3] = GET_SINGLE(UIManager)->LoadPrefab("ShortCut4").front();
	for (int i = 0; i < 4; i++)
	{
		uint32_t slotspellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[i]))->GetSpellType();
		static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellShortCutKeySlot[i]))->SetSpellType(slotspellType + ETOUI(SPELL_TYPE::B_NONE));
	}
	//static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellShortCutKeySlot[0]))->SetSpellType(ETOUI(SPELL_TYPE::B_BOMBARDA));

	m_SpellSlotStatic = GET_SINGLE(UIManager)->LoadPrefab("SpellSlotStatic");

	E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/SpellOpen.wav", SOUND_PLAY_DESC{
		.sBusID = SOUND_BUS::UI,
		.fVolume = 0.5f,
		.fPitch = 1.f,
		.iPriority = 64,
		.bLoop = false
		});

	E::CGameInstance::Get().SetMouseFix(false);
	SafeGetOBJ(*m_Cursor)->SetAlpha(1.f);
}

void CUIController::DeleteSpellType()
{
	for (auto hBtn : m_SpellBTNs)
	{
		PlayScaleAlphaDownDelete(hBtn);
	}
	for (auto& overlay : m_SpellLockOverlays)
		overlay = std::nullopt;
	for (auto hBG : m_SpellSlotStatic)
	{
		PlayFadeOutDelete(hBG);
	}


	for (int i = 0; i < 4; i++)
	{
		CSpellMeter* pSpellMeter = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellShortCutKeySlot[i]));
		CSpellMeter* pSpellSlot = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[i]));

		if(pSpellSlot->GetSpellType() != pSpellMeter->GetSpellType() - ETOUI(SPELL_TYPE::B_NONE))
			SetSpellType(i + 1, pSpellMeter->GetSpellType() - ETOUI(SPELL_TYPE::B_NONE));
		PlayScaleAlphaDownDelete(m_SpellShortCutKeySlot[i]);
	}

	E::CGameInstance::Get().SetMouseFix(true);
	SafeGetOBJ(*m_Cursor)->SetAlpha(0.f);
	GET_SINGLE(UIManager)->FadeInQuest(0.5f);

	E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/SpellClose.wav", SOUND_PLAY_DESC{
	.sBusID = SOUND_BUS::UI,
	.fVolume = 0.3f,
	.fPitch = 1.f,
	.iPriority = 64,
	.bLoop = false
		});
}

void CUIController::CreateDeathScene()
{
	m_isCreateDeathScene = true;

	m_Desolve = GET_SINGLE(UIManager)->LoadPrefab("Desolve").front();
	m_DeathDivider = GET_SINGLE(UIManager)->LoadPrefab("DeathDivider").front();
	m_DeathTxt = GET_SINGLE(UIManager)->LoadPrefab("DeathSceneTxt");

	PlayDividerUPWidth(m_DeathDivider);

	for (int i = 0; i < m_DeathTxt.size(); i++)
	{
		PlayAlphaUP(m_DeathTxt[i], 3.f - i * 0.5f, 1.f);
	}
		
	m_BeathButton[0] = GET_SINGLE(UIManager)->LoadPrefab("ButtonText1").front();
	m_BeathButton[1] = GET_SINGLE(UIManager)->LoadPrefab("ButtonText2").front();
	m_BeathButton[2] = GET_SINGLE(UIManager)->LoadPrefab("ButtonText3").front();
	PlayAlphaUP(m_BeathButton[0], 3.5f, 1.f);
	PlayAlphaUP(m_BeathButton[1], 3.75f, 1.f);
	PlayAlphaUP(m_BeathButton[2], 4.f, 1.f);

	m_GameOverMask = GET_SINGLE(UIManager)->LoadPrefab("GameOverMask").front();
	PlayAlphaUP(m_GameOverMask, 1.7f, 1.8f);
	GetSafeUI(m_GameOverMask)->SetSize({1024.f, 700.f});

	for (auto hUI : m_BeathButton)
	{
		SafeGetOBJ(hUI)->OnHoverEnter = GET_SINGLE(UIManager)->GetAction("TxtButtonScaleUp");
		SafeGetOBJ(hUI)->OnHoverExit = GET_SINGLE(UIManager)->GetAction("TxtButtonScaleDown");
		SafeGetOBJ(SafeGetOBJ(hUI)->GetChildren().front())->OnHoverEnter = GET_SINGLE(UIManager)->GetAction("TxtButtonColorUp");
		SafeGetOBJ(SafeGetOBJ(hUI)->GetChildren().front())->OnHoverExit = GET_SINGLE(UIManager)->GetAction("TxtButtonColorDown");
	}
	SafeGetOBJ(SafeGetOBJ(m_BeathButton[0])->GetChildren().front())->OnClickedAction = GET_SINGLE(UIManager)->GetFunc("ClearDeathScene");

	//SafeGetOBJ(m_PotionCount)->GetUIInfo().Color = { 0.f, 0.f, 0.f };
	PlayFadeOutOnly(m_PotionCount);
	E::CGameInstance::Get().SetMouseFix(false);
	SafeGetOBJ(*m_Cursor)->SetAlpha(1.f);

	E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/Death.wav", SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::UI,
			.fVolume = 0.5f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});

	/*********텍스트 안보이게**********/
	if (std::nullopt != m_MonsterHP && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		PlayFadeOutOnly(*m_MonsterHP);
}

void CUIController::SetHPMax(_float maxHP)
{
	if (nullptr != SafeGetOBJ(m_PlayerHP))
	{
		static_cast<CHPBar*>(SafeGetOBJ(m_PlayerHP))->SetMaxFill(maxHP);
	}
}


void CUIController::AddHP(_float amoutFill)
{
	if (nullptr != SafeGetOBJ(m_PlayerHP))
	{
		static_cast<CHPBar*>(SafeGetOBJ(m_PlayerHP))->AddFill(amoutFill);
	}
}

void CUIController::AddFinisher(_float amountFill)
{
	m_FinisherAmount += amountFill;
	SetFinisher(m_FinisherAmount);
}

void CUIController::SetFinisher(_float amountFill)
{
	m_FinisherAmount = amountFill;
	m_FinisherAmount = std::clamp(m_FinisherAmount, 0.0f, 100.f);
	if (nullptr != SafeGetOBJ(m_Finisher[0]))
	{
		static_cast<CHPBar*>(SafeGetOBJ(m_Finisher[0]))->SetCurrentFill(std::min(100.f, m_FinisherAmount * 10.f / 3.f));
		static_cast<CHPBar*>(SafeGetOBJ(m_Finisher[1]))->SetCurrentFill(std::min(100.f, (m_FinisherAmount - 100.f / 3.f) * 10.f / 3.f));
		static_cast<CHPBar*>(SafeGetOBJ(m_Finisher[2]))->SetCurrentFill(std::min(100.f, (m_FinisherAmount - 200.f / 3.f) * 10.f / 3.f));
	}
}

void CUIController::SetSpellType(uint32_t SlotNumber, uint32_t SpellType)
{
	CHandle SpellEffect = GET_SINGLE(UIManager)->LoadPrefab("SpellChoiceEffect").front();
	switch (SlotNumber)
	{
	case 1:
		{
			static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[0]))->SetSpellType(SpellType);
			_float2 pos = SafeGetOBJ(m_SpellSlot[0])->GetPos();
			SafeGetOBJ(SpellEffect)->SetPos(pos);
			break;
		}
	case 2:
		{
			static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[1]))->SetSpellType(SpellType);
			_float2 pos = SafeGetOBJ(m_SpellSlot[1])->GetPos();
			SafeGetOBJ(SpellEffect)->SetPos(pos);
			break;
		}
	case 3:
		{
			static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[2]))->SetSpellType(SpellType);
			_float2 pos = SafeGetOBJ(m_SpellSlot[2])->GetPos();
			SafeGetOBJ(SpellEffect)->SetPos(pos);
			break;
		}
	case 4:
		{
			static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[3]))->SetSpellType(SpellType);
			_float2 pos = SafeGetOBJ(m_SpellSlot[3])->GetPos();
			SafeGetOBJ(SpellEffect)->SetPos(pos);
			break;
		}
	defualt:
		break;
	}
	GET_SINGLE(UIManager)->SaveSpellSlot(SlotNumber, SpellType);
}

_bool CUIController::IsSpellUnlocked(SPELL_TYPE spellType) const
{
	return GET_SINGLE(UIManager)->IsSpellUnlocked(spellType);
}

void CUIController::SetSpellUnlocked(SPELL_TYPE spellType, _bool unlocked)
{
	(void)unlocked;
	for (size_t i = 0; i < SPELL_BUTTON_TYPES.size(); ++i)
	{
		if (SPELL_BUTTON_TYPES[i] != spellType)
			continue;
		if (i < m_SpellBTNs.size())
			RefreshSpellLockVisual(i);
		break;
	}
}

void CUIController::ApplySpellLockStates()
{
	for (size_t i = 0; i < SPELL_BUTTON_TYPES.size() && i < m_SpellBTNs.size(); ++i)
		RefreshSpellLockVisual(i);
}

void CUIController::RefreshSpellLockVisual(size_t spellButtonIndex)
{
	if (spellButtonIndex >= SPELL_BUTTON_TYPES.size() ||
		spellButtonIndex >= m_SpellBTNs.size())
		return;

	auto* button = E::CGameInstance::Get().
		GetGameObjectByHandleT<CButton>(m_SpellBTNs[spellButtonIndex]);
	if (!button)
		return;

	const _bool unlocked = GET_SINGLE(UIManager)->
		IsSpellUnlocked(SPELL_BUTTON_TYPES[spellButtonIndex]);
	button->SetSpellUnlocked(unlocked);

	// SpellTypeHoverEffect is a regular child UI, so it receives its own
	// ENTER/EXIT events independently from the spell button. Lock it
	// explicitly while the spell is unavailable, while leaving the parent
	// button interactive so the description popup can still be displayed.
	for (const CHandle childHandle : button->GetChildren())
	{
		auto* child = GetSafeUI(childHandle);
		if (!child || std::string_view(child->GetName()) != "SpellTypeHoverEffect")
			continue;

		child->SetInputLcok(!unlocked);
		if (!unlocked)
		{
			if (auto* tween = child->GetTweenCom())
				tween->ClearTweens();
			child->SetAlphaRatio(0.f);
			child->SetAlpha(0.f);
			child->CalcUICoord();
		}
	}

	auto& overlayHandle = m_SpellLockOverlays[spellButtonIndex];
	if (unlocked)
	{
		if (overlayHandle && GetSafeUI(*overlayHandle))
			GET_SINGLE(UIManager)->DeleteUIRecursive(*overlayHandle);
		overlayHandle = std::nullopt;
		return;
	}

	if (overlayHandle && GetSafeUI(*overlayHandle))
		return;

	const auto overlayObjects = GET_SINGLE(UIManager)->LoadPrefab("SpellLockFade");
	if (overlayObjects.empty())
		return;

	overlayHandle = overlayObjects.front();
	auto* overlay = GetSafeUI(*overlayHandle);
	if (!overlay)
	{
		overlayHandle = std::nullopt;
		return;
	}

	overlay->SetParent(button->GetHandle());
	button->AddChildren(*overlayHandle);
	overlay->SetLocalPos({ 0.f, 0.f });
	overlay->SetLocalScaleRatio(1.f);
	overlay->SetAlphaRatio(0.3f);
	overlay->GetUIInfo().WeightOffset = 1;
	for (const CHandle handle : overlayObjects)
	{
		if (auto* ui = GetSafeUI(handle))
			ui->SetInputLcok(true);
	}
	overlay->CalcUICoord();
}

uint32_t CUIController::GetSpellType(uint32_t SlotNumber)
{
	uint32_t spellType = 0;
	switch (SlotNumber)
	{
	case 1:
		spellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[0]))->GetSpellType();
		break;
	case 2:
		spellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[1]))->GetSpellType();
		break;
	case 3:
		spellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[2]))->GetSpellType();
		break;
	case 4:
		spellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[3]))->GetSpellType();
		break;
	defualt:
		break;
	}

	return spellType;
}

void CUIController::UseSpell(uint32_t SlotNumber)
{
	if (SlotNumber < 1u || SlotNumber > 4u)
		return;

	auto* pSpellSlot = static_cast<CSpellMeter*>(
		SafeGetOBJ(m_SpellSlot[SlotNumber - 1u]));
	if (!pSpellSlot || pSpellSlot->GetSpellType() == ETOUI(SPELL_TYPE::NONE) ||
		pSpellSlot->GetFillAmount() < 0.999f)
		return;

	const SPELL_TYPE spellType =
		static_cast<SPELL_TYPE>(pSpellSlot->GetSpellType());
	const char* dialogue = nullptr;

	switch (spellType)
	{
	case SPELL_TYPE::ASSIO:
		dialogue = "아씨오!";
		break;
	case SPELL_TYPE::DEPULSO:
		dialogue = "디펄쏘!";
		break;
	case SPELL_TYPE::DESENDO:
		dialogue = "디센도!";
		break;
	case SPELL_TYPE::BOMBARDA:
		dialogue = "봄바르다!";
		break;
	case SPELL_TYPE::LUMOS:
		dialogue = "루모스!";
		break;
	case SPELL_TYPE::REPARO:
		dialogue = "레파로!";
		break;
	case SPELL_TYPE::AVADAKEDAVRA:
		dialogue = "아바다 케다브라!";
		break;
	case SPELL_TYPE::CONFRINGO:
		dialogue = "콘프링고!";
		break;
	case SPELL_TYPE::DIFFINDO:
		dialogue = "디핀도!";
		break;
	case SPELL_TYPE::TRANSFORMATION:
		dialogue = "변환!";
		break;
	default:
		break;
	}

	pSpellSlot->StartCooldown();
	if (dialogue)
		GET_SINGLE(UIManager)->AddDialoguePopup("샤프교수", dialogue);
}

void CUIController::SetPotionCount(_float cnt)
{
	m_iPotionCNT = cnt;
	m_iPotionCNT = std::clamp(m_iPotionCNT, 0, 99);
	if (m_iPotionCNT < 10)
	{
		static_cast<CTextBox*>(SafeGetOBJ(m_PotionCount))->SetwText(L"  " + std::to_wstring(m_iPotionCNT));
	}
	else if (nullptr != SafeGetOBJ(m_PotionCount))
	{
		static_cast<CTextBox*>(SafeGetOBJ(m_PotionCount))->SetwText(std::to_wstring(m_iPotionCNT));
	}
}

void CUIController::AddPotionCount(_float cnt)
{
	m_iPotionCNT += cnt;
	m_iPotionCNT = std::clamp(m_iPotionCNT, 0, 99);
	if (nullptr != SafeGetOBJ(m_PotionCount))
	{
		static_cast<CTextBox*>(SafeGetOBJ(m_PotionCount))->SetwText(std::to_wstring(m_iPotionCNT));
	}
}

void CUIController::UsePotion()
{
	if (m_iPotionCNT <= 0)
		return;

	AddPotionCount(-1);
	AddHP(400.f);
}

void CUIController::TargetMonsterHP(CHandle monsterHandle)
{
	m_ReserveTargetHandle = monsterHandle;
	if (m_MonsterHP != std::nullopt && nullptr != SafeGetOBJ(*m_MonsterHP))
	{
		PlayMonsterHPDeleteCreate(*m_MonsterHP);
	}
	else
	{
		m_bMonsterHP = true;
	}
}

void CUIController::CreateMonsterHP()
{
	m_TargetHandle = m_ReserveTargetHandle;

	if (m_TargetHandle == std::nullopt || nullptr == E::CGameInstance::Get().GetGameObjectByHandleT<CMonster>(*m_TargetHandle))
		return;

	auto* pMonster = E::CGameInstance::Get().GetGameObjectByHandleT<CMonster>(*m_TargetHandle);
	if (std::nullopt == m_TargetHandle || nullptr == pMonster)
		return;

	if (pMonster->Get_CurrentHp() <= 0.f)
		return;

	const char* pPrefabName = "MonsterHP";
	if (pMonster->Is<CEnderDragon>())
		pPrefabName = "RanRockHP";
	else if (pMonster->Is<CBossTMB>())
		pPrefabName = "PensiveHP";

	auto loadedHandles = GET_SINGLE(UIManager)->LoadPrefab(pPrefabName);
	if (loadedHandles.empty())
		return;

	m_MonsterHP = loadedHandles.front();

	if (pMonster->Is<CSpider>())
	{
		const auto setSpiderName = [](auto&& self, CHandle handle) -> void
		{
			auto* pUI = GetSafeUI(handle);
			if (!pUI)
				return;

			if (std::string_view(pUI->GetName()) == "MonsterName")
			{
				if (auto* pTextBox = Engine::Cast<CTextBox>(pUI))
					pTextBox->SetwText(L"가시등 거미");
			}

			for (const CHandle childHandle : pUI->GetChildren())
				self(self, childHandle);
		};

		for (const CHandle rootHandle : loadedHandles)
			setSpiderName(setSpiderName, rootHandle);
	}
}

void CUIController::UpdateMonsterHP()
{
	if (m_TargetHandle != std::nullopt && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CMonster>(*m_TargetHandle))
	{
		auto* pMonster = E::CGameInstance::Get().GetGameObjectByHandleT<CMonster>(*m_TargetHandle);
		
		if (m_MonsterHP != std::nullopt && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		{
			static_cast<CHPBar*>(GetSafeUI(*m_MonsterHP))->SetMaxFill(static_cast<_float>(pMonster->Get_MaxHp()));
			static_cast<CHPBar*>(GetSafeUI(*m_MonsterHP))->SetCurrentFill(static_cast<_float>(pMonster->Get_CurrentHp()));
		}

	}
	else {
		if (m_MonsterHP != std::nullopt && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		{	
			static_cast<CHPBar*>(GetSafeUI(*m_MonsterHP))->SetMaxFill(static_cast<_float>(100.f));
			static_cast<CHPBar*>(GetSafeUI(*m_MonsterHP))->SetCurrentFill(static_cast<_float>(0.f));
		}
	}
}

void CUIController::DeleteMonsterHP()
{
	if(m_MonsterHP != std::nullopt && E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
	{
		const CHandle hMonsterHP = *m_MonsterHP;
		m_bMonsterHP = false;
		m_MonsterHP = std::nullopt;
		PlayMonsterHPDelete(hMonsterHP);
	}
}

void CUIController::AddMonsterHP(_float fill)
{
	if (std::nullopt == m_MonsterHP || nullptr == E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		return;
	static_cast<CHPBar*>(SafeGetOBJ(*m_MonsterHP))->AddFill(fill);
}

void CUIController::ClearDeathScene()
{
	GET_SINGLE(UIManager)->CreateFadeInSceneChange(0.f, 1.f, LEVEL::BOSS_CHARLES_ROOKWOOD);

	//PlayFadeOutDelete(m_Desolve);
	//PlayFadeOutDelete(m_DeathDivider);
	//PlayFadeOutDelete(m_DeathTxt[0]);
	//PlayFadeOutDelete(m_DeathTxt[1]);
	//
	//for (auto hUI : m_BeathButton)
	//{
	//	PlayFadeOutDelete(hUI);
	//}
	//
	////SafeGetOBJ(m_PotionCount)->GetUIInfo().Color = {1.f, 1.f, 1.f};
	//PlayFadeInOnly(m_PotionCount);
	//
	//PlayFadeOutDelete(m_GameOverMask);
	//m_isCreateDeathScene = false;

	if(std::nullopt != m_MonsterHP && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		PlayFadeInOnly(*m_MonsterHP);
	E::CGameInstance::Get().SetMouseFix(true);
	SafeGetOBJ(*m_Cursor)->SetAlpha(0.f);
}

E::CUIObject* CUIController::SafeGetOBJ(CHandle pHandle)
{
	if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle))
		return E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle);

	return nullptr;
}

void CUIController::PlayScaleAlphaDownDelete(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float scaleRatio = pBtn->GetScaleRatio();
	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(scaleRatio, 0.f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetScaleRatio(currentValue);
			pBtn->CalcUICoord();
		}, [pHandle]() {
			if (auto pObj = GetSafeUI(pHandle)) GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			}, EEaseType::EaseOutQuad);

		pTween->PlayTween(Alpah, 0.f, 0.2f,
			[pBtn](float currentValue) {
				pBtn->SetAlpha(currentValue);
				pBtn->CalcUICoord();
			}, nullptr, EEaseType::EaseOutQuad);
}

void CUIController::PlayFadeOutDelete(CHandle pHandle, float delaytime, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(1.f, 0.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, [pHandle]() {
			if (auto pObj = GetSafeUI(pHandle)) GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			}, EEaseType::EaseOutQuad, delaytime);
}

void CUIController::PlayFadeOutOnly(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(1.f, 0.f, 1.f,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, 1.f);
}

void CUIController::PlayFadeInOnly(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(0.f, 1.f, 0.5f,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, 0.2f);
}

void CUIController::PlayMonsterHPDelete(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);
	_float scaleRatio = pBtn->GetScaleRatio();
	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(scaleRatio, 0.5f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetScaleRatio(currentValue);
			pBtn->CalcUICoord();
		}, nullptr, EEaseType::EaseOutQuad);

	pTween->PlayTween(Alpah, 0.f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
			pBtn->CalcUICoord();
		}, [pHandle, this]() {
			GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			}, EEaseType::EaseOutQuad);
}

void CUIController::PlayMonsterHPDeleteCreate(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);
	_float scaleRatio = pBtn->GetScaleRatio();
	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(scaleRatio, 0.5f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetScaleRatio(currentValue);
			pBtn->CalcUICoord();
		}, nullptr, EEaseType::EaseOutQuad);

	pTween->PlayTween(Alpah, 0.f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
			pBtn->CalcUICoord();
		}, [pHandle, this]() {
			GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			m_bMonsterHP = true;
			}, EEaseType::EaseOutQuad);
}

void CUIController::PlayDividerUPWidth(CHandle pHandle)
{
	CUIObject* pUI = SafeGetOBJ(pHandle);
	auto pTween = pUI->GetTweenCom();

	//pUI->SetInputLcok(true);

	_float width = pUI->GetSize().x;
	_float height = pUI->GetSize().y;
	_float Alpah = pUI->GetAlpha();

	pTween->PlayTween(256.f, width + 256.f, 1.f,
		[pUI, height](float currentValue) {
			pUI->SetSize({ currentValue, height });
			pUI->CalcUICoord();
		}, nullptr, EEaseType::Linear, 3.f);

	pTween->PlayTween(0.f, 1.f, 1.f,
		[pUI](float currentValue) {
			pUI->SetAlpha(currentValue);
			pUI->CalcUICoord();
		}, nullptr, EEaseType::Linear, 3.f);

}

void CUIController::PlayAlphaUP(CHandle pHandle, float delaytime, float playTime)
{
	CUIObject* pUI = SafeGetOBJ(pHandle);
	auto pTween = pUI->GetTweenCom();

	pUI->SetInputLcok(true);

	_float Alpah = pUI->GetAlpha();

	pTween->PlayTween(0.f, 1.f, playTime,
		[pUI](float currentValue) {
			pUI->SetAlpha(currentValue);
			pUI->CalcUICoord();
		}, nullptr, EEaseType::Linear, delaytime);
}

void CUIController::Free()
{
	if (m_hSpellMiniGame)
	{
		if (auto* miniGame = E::CGameInstance::Get().
			GetGameObjectByHandleT<CSpellMiniGame>(*m_hSpellMiniGame))
		{
			miniGame->SetPendingDestroyCascade();
		}
		m_hSpellMiniGame = std::nullopt;
	}

	if (m_iQuestUIListenerID != 0)
	{
		E::CGameInstance::Get().
			EventUnsubscribe<FQuestUIGroupChanged>(m_iQuestUIListenerID);
		m_iQuestUIListenerID = 0;
	}

	m_hMiniMap = std::nullopt;
	CGameObject::Free();
}

E::UPtr<CUIController> CUIController::Create()
{
	auto pInstance = E::ToUPtr(new CUIController{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CUIController");
		return nullptr;
	}
	return  pInstance;
}


E::UPtr<E::CPrototype> CUIController::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CUIController{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUIController");
		return nullptr;
	}

	return pInstance;
}
