#pragma once
#include "Client_Defines.h"
#include "UIObject.h"
#include "UI_Structs.h"
#include "WandShop.h"
#include <array>

NS_BEGIN(Client)

static Engine::CUIObject* GetSafeUI(CHandle handle)
{
	return E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(handle);
}

class UIManager
{
	DECLARE_SINGLE(UIManager)

	~UIManager();

	void	Update(_float fTimeDelta);

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

public:
	void Initialize(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	void InitializeActions();
	void InitializeFunc();
	void UpdateRootUIHandles();

public:
	std::function<void(CUIObject* pCaller)> GetAction(const std::string& actionName);
	std::function<void(std::string text)> GetFunc(const std::string& funcName);

	std::vector<std::string>* GetEventNames() { return &m_vEventNames; }
	std::vector<std::string>* GetFuncNames(){ return &m_vFuncNames; }

	std::vector<CHandle> GetRootUIHandles() { return rootUIHandles; }
	std::optional<CHandle>  GetUIController() { return m_UIController; }
	void SetUIController(std::optional<CHandle> hController) { m_UIController = hController; }

	/*******페이드 인아웃******/
	void CreateFadeIn(float delay = 0.f, float playtime = 0.5f);
	void CreateFadeOut(float delay = 0.f, float playtime = 0.5f);
	void CreateFadeInSceneChange(float delay = 0.f, float playtime = 1.f, LEVEL level = LEVEL::LOGO);
	void PlayFadeInAll2DUI(float delay = 0.f, float playtime = 0.5f);
	void PlayFadeOutAll2DUI(float delay = 0.f, float playtime = 0.5f);

	/********스펠 잠금 및 슬롯 영속 상태********/
	_bool IsSpellUnlocked(SPELL_TYPE spellType) const;
	void SetSpellUnlocked(SPELL_TYPE spellType, _bool unlocked);
	uint32_t GetSavedSpellSlot(uint32_t slotNumber) const;
	void SaveSpellSlot(uint32_t slotNumber, uint32_t spellType);
	_bool HasInitializedSpellSlots() const { return m_bSpellSlotsInitialized; }

	/********데미지 폰트***********/
	void CreateDamageFont(uint32_t damage, CHandle targetMonster,_bool isCritical = false);

	/********액티브 버튼***********/
	void CreateActiveButton(CHandle handle, _ubyte KeyType);
	void RemoveActiveButton(CHandle handle, _bool fadeOut = true);

	/********대화 팝업***********/
	void AddDialoguePopup(const std::string& speaker, const std::string& message);
	void ClearDialoguePopups(_bool immediate = false);

	/********NPC 말풍선***********/
	void ShowNPCSpeechBubble(CHandle npcHandle, const std::string& message,
		_float duration = 5.f,
		const _float3& worldOffset = { 0.f, 2.2f, 0.f });
	void RemoveNPCSpeechBubble(CHandle npcHandle, _bool fadeOut = true);
	void ClearNPCSpeechBubbles(_bool immediate = false);

	/********퀘스트 안내***********/
	// Quest UI가 없으면 생성하고, 이미 표시 중이면 텍스트 전환 모션으로 교체한다.
	void CreateOrChangeQuest(const std::string& questText);
	void DeleteQuest();
	void FadeOutQuest(float playtime = 0.3f);
	void FadeInQuest(float playtime = 0.5f);

	/********레이스 시작 타이머***********/
	void StartRaceStartTimer();
	_bool IsRaceStartTimerPlaying() const { return m_bRaceStartTimerPlaying; }

	/********레이스 미니게임***********/
	void StartRaceMiniGame();
	void AddRaceMiniGameCoin(uint32_t amount = 1u);
	void FinishRaceMiniGame();
	uint32_t GetRaceMiniGameCoinCount() const
	{
		return m_iRaceMiniGameCoinCount;
	}

	/********지팡이 상점***********/
	void OpenWandShop();

	void OpenWandShopWorld(
		CHandle targetHandle,
		const _float3& positionOffset,
		const _float3& rotationOffsetDegrees); // 3D월드 상점

	void OpenWandShopPage(uint32_t pageIndex);
	void CloseWandShop();
	_float2 GetUIInteractionMousePosition() const;
	_bool IsWandShopWorldMode() const { return m_bWandShopWorldMode; }
public:
	std::optional<CHandle> RootUIPicking();

private:
	std::map<std::string, std::function<void(class CUIObject*)>> m_EventMap;
	std::map<std::string, std::function<void(std::string name)>> m_FuncMap;
	std::unordered_map<std::string, std::wstring> m_StringTable;
	std::vector<CHandle> rootUIHandles;
	struct UI_HANDLE_HASH
	{
		size_t operator()(const CHandle& handle) const noexcept
		{
			const size_t indexHash = std::hash<size_t>{}(handle.GetIndex());
			const size_t generationHash = std::hash<uint32_t>{}(handle.GetGeneration());
			return indexHash ^ (generationHash + 0x9e3779b9u + (indexHash << 6u) + (indexHash >> 2u));
		}
	};
	std::unordered_map<CHandle, _float, UI_HANDLE_HASH> m_2DUIRestoreAlpha{};
	std::unordered_map<CHandle, _bool, UI_HANDLE_HASH> m_2DUIRestoreInputLock{};
	std::unordered_map<CHandle, _float, UI_HANDLE_HASH> m_SpellMeterRestoreScale{};
	std::array<_bool, static_cast<size_t>(SPELL_TYPE::B_NONE)> m_SpellUnlockStates = []
	{
		std::array<_bool, static_cast<size_t>(SPELL_TYPE::B_NONE)> states{};
		states.fill(true);
		states[static_cast<size_t>(SPELL_TYPE::FLIPENDO)] = false;
		states[static_cast<size_t>(SPELL_TYPE::TRANSFORMATION)] = false;
		states[static_cast<size_t>(SPELL_TYPE::EXPELLIARMUS)] = false;
		states[static_cast<size_t>(SPELL_TYPE::INCENDIO)] = false;
		states[static_cast<size_t>(SPELL_TYPE::DISILLUSIONMENT)] = false;
		states[static_cast<size_t>(SPELL_TYPE::AVADAKEDAVRA)] = false;
		states[static_cast<size_t>(SPELL_TYPE::CRUCIO)] = false;
		states[static_cast<size_t>(SPELL_TYPE::IMPERIO)] = false;
		return states;
	}();
	std::array<uint32_t, 4> m_SavedSpellSlots{};
	_bool m_bSpellSlotsInitialized{ false };

	// 애니메이션 함수, 실행 함수들 이름
	std::vector<std::string> m_vEventNames;
	std::vector<std::string> m_vFuncNames;

	std::string m_CurrentLevel;

	std::vector<CHandle> m_vLoadPrefabRoot{};
	std::optional<CHandle> m_UIController = std::nullopt;
	std::vector<ACTIVE_BUTTON_INFO> m_ActiveButtons{};
	std::vector<DIALOGUE_POPUP_INFO> m_DialoguePopups{};
	std::vector<NPC_SPEECH_BUBBLE_INFO> m_NPCSpeechBubbles{};
	std::optional<CHandle> m_hQuestRoot{};
	std::optional<CHandle> m_hQuestText{};
	std::optional<CHandle> m_hQuestTargetIcon{};
	std::string m_CurrentQuestText{};
	_float2 m_QuestTextBaseLocalPos{};
	_float2 m_QuestTargetIconBaseLocalPos{};
	_float m_fDialogueTargetWidth{};
	std::vector<CHandle> m_RaceStartTimerRoots{};
	std::optional<CHandle> m_hRaceStartTimerFrame{};
	std::optional<CHandle> m_hRaceStartTimerNumberPad{};
	std::optional<CHandle> m_hRaceStartTimerText{};
	std::optional<CHandle> m_hRaceStartTimerFire{};
	std::optional<CHandle> m_hRaceStartTimerFlagR{};
	std::optional<CHandle> m_hRaceStartTimerFlagL{};
	_float m_fRaceStartTimerElapsed{};
	_float m_fRaceStartTimerFrameBaseScale{ 1.f };
	_float m_fRaceStartTimerNumberPadBaseScale{ 1.f };
	_float2 m_RaceStartTimerTextBaseLocalPos{};
	_float m_fRaceStartTimerFireBaseAlpha{ 1.f };
	_float m_fRaceStartTimerFlagRBaseAlpha{ 1.f };
	_float m_fRaceStartTimerFlagLBaseAlpha{ 1.f };
	_bool m_bRaceStartTimerPlaying{ false };

	enum class RACE_MINIGAME_PHASE : uint8_t
	{
		NONE,
		COUNTDOWN,
		RACING,
		RESULT
	};
	RACE_MINIGAME_PHASE m_eRaceMiniGamePhase{ RACE_MINIGAME_PHASE::NONE };
	std::vector<CHandle> m_RaceBoardRoots{};
	std::vector<CHandle> m_RaceResultRoots{};
	std::optional<CHandle> m_hRaceBoardTimerText{};
	std::optional<CHandle> m_hRaceBoardCoinText{};
	std::optional<CHandle> m_hRaceResultCoinText{};
	_float m_fRaceMiniGameElapsed{};
	uint32_t m_iRaceMiniGameCoinCount{};
	CWandShop m_WandShop{};
	_bool m_bWandShopWorldMode{ false };
	_float4x4 m_WandShopPanelWorld{};
	_float2 m_WandShopPanelMousePosition{ -FLT_MAX, -FLT_MAX };
	_bool m_bWandShopPanelMouseHit{ false };

	void UpdateActiveButtons();
	void UpdateDialoguePopups(_float fTimeDelta);
	void RefreshDialoguePopupLayout();
	void UpdateNPCSpeechBubbles(_float fTimeDelta);
	void UpdateRaceStartTimer(_float fTimeDelta);
	void UpdateRaceMiniGame(_float fTimeDelta);
	void BeginRaceBoard();
	void ClearRaceMiniGameUI();
	void PlayRaceRootsFadeIn(const std::vector<CHandle>& roots,
		_float playtime = 0.3f);
	void UpdateWandShopWorldMousePosition();
	// 피킹용
	_bool PtInRect(const UI_INFO& selectInfo, _float scaleRatio);
public:
	std::vector<CHandle> LoadPrefab(std::string name, std::string g_BasePath = "./Resources/SampleClient/UIData/Prefabs/");
	E::CUIObject* LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent);
	void DeleteUIRecursive(std::optional<CHandle> targetHandle);
	void PlayFadeOutDelete(CHandle pHandle, float delay = 1.f, float playtime = 5.f);
	void PlayFadeIn(CHandle pHandle, float delay = 0.f, float playtime = 5.f);
	void PlayOnlyFadeIn(CHandle pHandle, float delay = 0.f, float playtime = 5.f);

private:
	friend class CWandShop;
	std::vector<CHandle> LoadPrefabFiltered(const std::string& name,
		const std::string& basePath,
		const std::function<_bool(const nlohmann::ordered_json&)>& predicate);
	/*************페이드인아웃****************/
	CUIObject* SafeGetOBJ(CHandle pHandle)
	{
		return E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle);
	}
	void PlayScaleDown(CHandle pHandle, float delay = 1.f, float playtime = 5.f);
	void PlayPosUP(CHandle pHandle, float delay = 1.f, float playtime = 5.f);
	void PlayFadeInChange(CHandle pHandle, LEVEL level, float delay = 0.f, float playtime = 3.f);

private:
	std::string g_BasePath = "./Resources/Client/UIData/Prefabs/";
	std::optional<CHandle> m_rootHandle = std::nullopt;
};

NS_END
