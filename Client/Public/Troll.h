#pragma once
#include "Monster.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComPxBoxCollider;
NS_END

enum class TROLL_SKILL { SMASH,DOLJIN, END };

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
	void						ReadySound() override;

public:
	_string						Get_SkillName(ATTMON SkillNode)override;
	void						Set_StateFinished(_bool bFinished);
	void						Set_Break(_bool bHit) { m_bIsBreak = bHit; }

	_bool						Is_StateFinished();
	_bool						Check_Table(PLAYER_SKILL_TYPE eType) override;

	void						Set_AttTable(ATTMON eType, _float2 fSkillRatio) override;
	TROLL_SKILL_INFO&			Get_SkillInfo(TROLL_SKILL eType) { return m_SkillHandle[ETOUI(eType)]; }
	const _string&				Get_SkillName(TROLL_SKILL eType) { return m_EffectNames[ETOUI(eType)]; }
	void						Destory_Child() override;
	const _float				Get_Damage() override;
	void						OnCollisionEnter(
		CGameObject* pObj,
		const PX_ON_COLLISION_DATA& info) override;
private:
	HRESULT					InitializeChargeCollider();
	void						UpdateChargeColliderState();
	void						Update_BBToFsm();
	void						Flag_Check(_float fTimeDelta) override;
	_bool						BreakSkillType(PLAYER_SKILL_TYPE eType);
	void						Set_Damage(TROLL_SKILL eType);
	void						InitializeEffects();
	void						Stuck() override;
private:
	class CMon_State*	m_pFsm{ nullptr };

	_string				m_EffectNames[ETOUI(TROLL_SKILL::END)]{};
	TROLL_SKILL_INFO	m_SkillHandle[ETOUI(TROLL_SKILL::END)]{};
	TROLL_SKILL			m_eDragonSkill{};
	CComPxBoxCollider*	m_pChargeBodyCollider{};
	PX_FILTER_DESC		m_tDefaultCCTFilter{};
	_bool				m_bIsBreak{ false }, m_bActiveSKill{ false };
	_float				m_fTick{ 0.f };
	_bool				m_bChargeBodyColliderEnabled{};
	static constexpr uint32_t CHARGE_BODY_SHAPE_INDEX = 100u;

public:
	static E::UPtr<CTroll> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
