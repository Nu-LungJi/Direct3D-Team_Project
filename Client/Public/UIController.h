#pragma once

#include "Client_Defines.h"
#include "UI_Enums.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
class CUIObject;
NS_END

NS_BEGIN(Client)


class CUIController final : public E::CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CUIController, E::CGameObject)

private:
	CUIController();
	~CUIController() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

	// ******************** 외부 호출용 함수 ***************************//
public:
	void CreatePlayScreen();
	void CreateSpellType();
	void DeleteSpellType();

	void AppearPlayScreen();
	void DisappearPlayeSceen();

	// ******** HP
	void SetHPMax(_float maxHP);
	void AddHP(_float amoutFill);
	
	// ********* Finisher
	void AddFinisher(_float amountFill);
	void MinusFinisher(_float amountFill);
	void SetFinisher(_float amountFill);

	// ********* SpellSlot
	void SetSpellType(uint32_t SlotNumber, uint32_t SpellType);
	uint32_t GetSpellType(uint32_t SlotNumber);
	void UseSpell(uint32_t SlotNumber);

	// ********* Potion
	void SetPotionCount(_float cnt);
	void AddPotionCount(_float cnt);
	void UsePotion();

	// ********** PickIcon
	void SetTargetIcon(uint32_t type) { m_TargetSpellType = type; }
	uint32_t GetTargetIcon() { return m_TargetSpellType; }

private: // ************ 계속 바뀌는 유아이 ******************* //
	/*************플레이 화면 유아이******************/
	CHandle m_SpellSlot[4] = {};
	CHandle m_PlayerHP{};
	CHandle m_Finisher[3] = {};
	CHandle m_FinisherIcon{};
	CHandle m_PotionCount{};
	std::vector<CHandle> m_PlaySceneStatic{};

	CHandle m_MonsterHP{};

	/*****************스펠 슬롯 유아이********************/
	CHandle m_SpellShortCutKeySlot[4] = {};
	std::vector<CHandle> m_SpellBTNs = {};
	uint32_t m_TargetSpellType = ETOUI(SPELL_TYPE::B_NONE);
	std::vector<CHandle> m_SpellSlotStatic{};

private:
	_bool ActivePlayScreen{ false };
	_bool ActiveShortCutSlot{ false };

	// Finisher
	_float m_FinisherAmount{ 0.f };
	float targetAmount{0.f};

	// Potion
	int m_iPotionCNT{23};

	//*********내부함수*************//
private:
	CUIObject* SafeGetOBJ(CHandle pHandle);

	/**********모션************/
	void PlayScaleAlphaDownDelete(CHandle pHandle);
	void PlayFadeOutDelete(CHandle pHandle);
public:
	static E::UPtr<CUIController> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
