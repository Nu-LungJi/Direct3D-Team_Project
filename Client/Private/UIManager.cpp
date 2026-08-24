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

NS_USING(Client)

namespace
{
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
	UpdateWandShopWorldMousePosition();
	UpdateActiveButtons();
	UpdateDialoguePopups(fTimeDelta);
	UpdateNPCSpeechBubbles(fTimeDelta);
	m_WandShop.Update(*this, fTimeDelta);
}

void UIManager::UpdateWandShopWorldMousePosition()
{
	m_bWandShopPanelMouseHit = false;
	m_WandShopPanelMousePosition = { -FLT_MAX, -FLT_MAX };
	if (!m_bWandShopWorldMode)
		return;

	// Picking must use the same camera that projects the world RTT panel.
	auto* camera = E::CGameInstance::Get().GetCamera("PlayerCamera");
	if (!camera)
		camera = E::CGameInstance::Get().GetActiveCamera();
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
	CHandle hBG = GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front();
	PlayFadeIn(hBG, delay, playtime);
}

void UIManager::CreateFadeOut(float delay, float playtime)
{
	CHandle hBG = GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front();
	PlayFadeOutDelete(hBG, delay, playtime);
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
	desc.fSizeX = 1.5f;
	desc.fSizeY = 1.5f;
	desc.fAlpha = 1.f;
	desc.Text = L"";
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
		pDamageFont->SetSize({ 1.5f, 1.5f });
		pDamageFont->SetScaleRatio(1.25f);
	}
	else
	{
		pDamageFont->SetColor({ 1.f, 1.f, 1.f });
		pDamageFont->SetSize({1.2f, 1.2f});
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
	desc.fSizeX = 48.f;
	desc.fSizeY = 48.f;
	desc.fAlpha = 0.f;
	desc.ResWeight = 950;
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

	if (!root || !text)
	{
		m_hQuestRoot = std::nullopt;
		m_hQuestText = std::nullopt;

		const auto roots = LoadPrefab("Quest");
		if (roots.empty())
			return;

		root = FindUIByNameRecursive(roots, "QuestFrame");
		if (auto* textUI = FindUIByNameRecursive(roots, "QuestText"))
		{
			text = E::CGameInstance::Get().
				GetGameObjectByHandleT<CTextBox>(textUI->GetHandle());
		}
		if (!root || !text)
		{
			for (const CHandle rootHandle : roots)
				DeleteUIRecursive(rootHandle);
			return;
		}

		m_hQuestRoot = root->GetHandle();
		m_hQuestText = text->GetHandle();
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
				GET_SINGLE(UIManager)->PlayFadeIn(
					rootHandle, 0.f, 0.3f);
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
}

void UIManager::DeleteQuest()
{
	if (m_hQuestRoot && GetSafeUI(*m_hQuestRoot))
		PlayFadeOutDelete(*m_hQuestRoot, 0.f, 0.3f);

	m_hQuestRoot = std::nullopt;
	m_hQuestText = std::nullopt;
	m_CurrentQuestText.clear();
	m_QuestTextBaseLocalPos = {};
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
	const _float3& rotationOffsetDegrees)
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
	const _matrix panelWorld =
		XMMatrixScaling(PANEL_WIDTH, PANEL_HEIGHT, 1.f) *
		rotationOffset * targetPanelPose;

	_float4x4 storedPanelWorld{};
	XMStoreFloat4x4(&storedPanelWorld, panelWorld);

	m_bWandShopWorldMode = true;
	m_WandShopPanelWorld = storedPanelWorld;
	// The placement is now confirmed, so use the scene depth buffer and let
	// walls/props occlude the physical panel naturally.
	E::CGameInstance::Get().SetUI3DPanel(storedPanelWorld, true, false);
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

void UIManager::PlayFadeOutDelete(CHandle pHandle, float delay, float playtime)
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

void UIManager::PlayFadeOutAll2DUI(float delay, float playtime)
{
	UpdateRootUIHandles();

	for (const CHandle handle : rootUIHandles)
	{
		auto* pUI = GetSafeUI(handle);
		if (!pUI || !pUI->GetActive() || !pUI->GetVisible() ||
			pUI->GetResolvedRenderGroup() != RENDERGROUP::UI)
			continue;

		// FadeIn 때 복원할 각 UI의 원래 상태를 FadeOut 시작 시점에 보관한다.
		m_2DUIRestoreAlpha[handle] = pUI->GetAlpha();
		m_2DUIRestoreInputLock[handle] = pUI->GetInputLcok();
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
			m_SpellMeterRestoreScale[handle] = startScale;
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
	UpdateRootUIHandles();

	for (const CHandle handle : rootUIHandles)
	{
		auto* pUI = GetSafeUI(handle);
		if (!pUI || !pUI->GetActive() || !pUI->GetVisible() ||
			pUI->GetResolvedRenderGroup() != RENDERGROUP::UI)
			continue;

		const auto alphaIt = m_2DUIRestoreAlpha.find(handle);
		const _float targetAlpha = alphaIt != m_2DUIRestoreAlpha.end() ? alphaIt->second : pUI->GetAlpha();

		if (auto* pTween = pUI->GetTweenCom())
		{
			const _float startAlpha = pUI->GetAlpha();
			pTween->PlayTween(startAlpha, targetAlpha, playtime,
				[handle](_float value)
				{
					if (auto* pTarget = GetSafeUI(handle))
						pTarget->SetAlpha(value);
				}, [this, handle]()
				{
					if (auto* pTarget = GetSafeUI(handle))
					{
						const auto lockIt = m_2DUIRestoreInputLock.find(handle);
						pTarget->SetInputLcok(lockIt != m_2DUIRestoreInputLock.end() ? lockIt->second : false);
					}
				}, EEaseType::EaseOutQuad, delay);
		}
	}

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

