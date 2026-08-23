#pragma once
#include "Client_Defines.h"
#include "UIObject.h"
#include "UI_Structs.h"
#include "WandShop.h"

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

	/********지팡이 상점***********/
	void OpenWandShop();
	void OpenWandShopPage(uint32_t pageIndex);
	void CloseWandShop();

public:
	std::optional<CHandle> RootUIPicking();

private:
	std::map<std::string, std::function<void(class CUIObject*)>> m_EventMap;
	std::map<std::string, std::function<void(std::string name)>> m_FuncMap;
	std::unordered_map<std::string, std::wstring> m_StringTable;
	std::vector<CHandle> rootUIHandles;

	// 애니메이션 함수, 실행 함수들 이름
	std::vector<std::string> m_vEventNames;
	std::vector<std::string> m_vFuncNames;

	std::string m_CurrentLevel;

	std::vector<CHandle> m_vLoadPrefabRoot{};
	std::optional<CHandle> m_UIController = std::nullopt;
	std::vector<ACTIVE_BUTTON_INFO> m_ActiveButtons{};
	std::vector<DIALOGUE_POPUP_INFO> m_DialoguePopups{};
	std::vector<NPC_SPEECH_BUBBLE_INFO> m_NPCSpeechBubbles{};
	_float m_fDialogueTargetWidth{};
	CWandShop m_WandShop{};

	void UpdateActiveButtons();
	void UpdateDialoguePopups(_float fTimeDelta);
	void RefreshDialoguePopupLayout();
	void UpdateNPCSpeechBubbles(_float fTimeDelta);
	// 피킹용
	_bool PtInRect(const UI_INFO& selectInfo, _float scaleRatio);
public:
	std::vector<CHandle> LoadPrefab(std::string name, std::string g_BasePath = "./Resources/SampleClient/UIData/Prefabs/");
	E::CUIObject* LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent);
	void DeleteUIRecursive(std::optional<CHandle> targetHandle);
	void PlayFadeOutDelete(CHandle pHandle, float delay = 1.f, float playtime = 5.f);
	void PlayFadeIn(CHandle pHandle, float delay = 0.f, float playtime = 5.f);

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
