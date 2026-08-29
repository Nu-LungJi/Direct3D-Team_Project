#pragma once
#include "Monster.h"
#include "Client_Defines.h"
enum class SPIDER_SKILL { AB, END };

typedef struct stractiveskilltablespider
{
	_string		  SkillName{};
	_float		  fLifeTime{}, fDist{};
	int32_t		  iBoneOffset{};
	SPIDER_SKILL eType{};
}EDG_SPIDER_DESC;

NS_BEGIN(Client)

class CSpider final : public CMonster
{
public:
	DECLARE_DERIVED_TYPE(CSpider, CMonster)

public:
	typedef struct tagSpiderDesc : public CMonster::MONSTER_DESC
	{
		_float3 vPatrollStart{};
		_float3 vPatrollEnd{};

	}SPIDER_DESC;

private:
	CSpider();
	~CSpider() override;

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

public:
	_string						Get_SkillName(ATTMON SkillNode)override;

	void						Set_StateFinished(_bool bFinished);
	void						Set_Break(_bool bHit) { m_bIsBreak = bHit; }

	_bool						Is_StateFinished();
	void						Set_Gravity(_bool bGravity);

	_bool						Check_Table(PLAYER_SKILL_TYPE eType) override;

	void						Set_AttTable(ATTMON eType, _float2 fSkillRatio) override;
	const _string&				Get_SkillNmae(SPIDER_SKILL eType) { return m_EffectNames[ETOUI(eType)]; }
	const _float				Get_Damage() override;
private:
	void						ReadySound() override;
	void						Update_BBToFsm();
	void						Flag_Check(_float fTimeDelta) override;
	_bool						BreakSkillType(PLAYER_SKILL_TYPE eType);
	void						Stuck() override;
private:
	class CMon_State* m_pFsm{ nullptr };

	_string			m_EffectNames[ETOUI(SPIDER_SKILL::END)];
	_bool			m_bIsBreak{ false }, m_bActiveSKill{ false }, m_bDebug{ false };
	_float						m_fTick{};
	_string						m_WayName{};
	std::list<_float3>			m_DebugPoint;
public:
	static E::UPtr<CSpider> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
