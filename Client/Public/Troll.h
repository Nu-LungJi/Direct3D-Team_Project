#pragma once
#include "Monster.h"
#include "Client_Defines.h"
enum class TROLL_SKILL { BOOM, END };

NS_BEGIN(Client)
typedef struct strtrollskillInfo
{
	CHandle handle{};
	_bool	bPool;
	int32_t iBoneIndex{};
	_string LevelTag{};
	PROTO_GAMEOBJECT ProtoTag;
	_string NameTag{};
	int32_t iOffsetBoneIndex{ -1 };
	TROLL_SKILL eType{ TROLL_SKILL::END };

}TROLL_SKILL_INFO;
class CTroll final : public CMonster
{
public:
	DECLARE_DERIVED_TYPE(CTroll, CMonster)

public:
	typedef struct tagDragonDesc : public CMonster::MONSTER_DESC
	{

	}TROLL_DESC;

private:
	CTroll();
	~CTroll() override;

public:
	void UpdateGUI() override;
public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;
	HRESULT						Ready_Fsm(const _string& LevelTag);
	HRESULT						Ready_Skill(const _string& LevelTag);
	void						Ready_BBKeyValue();
	void						ReadySound();

public:
	_string						Get_SkillName(ATTMON SkillNode)override;
	void						Set_StateFinished(_bool bFinished);
	void						Set_Break(_bool bHit) { m_bIsBreak = bHit; }

	_bool						Is_StateFinished();
	_bool						Check_Table(PLAYER_SKILL_TYPE eType) override;

	void						Set_AttTable(ATTMON eType, _float2 fSkillRatio) override;
	void						Set_Dissolve(_float fDissolve) { m_fDissolve = fDissolve; }
	TROLL_SKILL_INFO&			Get_SkillInfo(TROLL_SKILL eType) { return m_SkillHandle[ETOUI(eType)]; }
	const _string&				Get_SkillNmae(TROLL_SKILL eType) { return m_EffectNames[ETOUI(eType)]; }
	void						Set_EndGame() { m_bEndGame = true; }
private:
	void						Update_BBToFsm();
	void						Flag_Check(_float fTimeDelta) override;
	_bool						BreakSkillType(PLAYER_SKILL_TYPE eType);

	void						InitializeEffects();
	void						Stuck() override;
private:
	class CMon_State* m_pFsm{ nullptr };

	_string			m_EffectNames[ETOUI(TROLL_SKILL::END)]{};
	TROLL_SKILL_INFO m_SkillHandle[ETOUI(TROLL_SKILL::END)]{};
	TROLL_SKILL		m_eDragonSkill{};
	_bool			m_bIsBreak{ false }, m_bActiveSKill{ false }, m_bDebug{ false }, m_bPopup{ false }, m_bPopupL{ false }, m_bEndGame{ false };

	_string						m_WayName{};
public:
	static E::UPtr<CTroll> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
