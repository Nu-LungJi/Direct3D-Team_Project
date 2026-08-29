#pragma once
#include "Client_Defines.h"

NS_BEGIN(Client)
class CSkillTarget 
{
public:
	DECLARE_RUNTIME_TYPE(CSkillTarget)

	virtual ~CSkillTarget() = default;
public:
	virtual _bool Check_Table(PLAYER_SKILL_TYPE) = 0;
	// 전투 타깃 탐색은 물리 레이어와 별개로 이 정책을 확인한다.
	// 몬스터는 기본적으로 허용하고, 비전투 NPC 계열은 기반 클래스에서 차단한다.
	virtual _bool CanBePlayerCombatTarget() const { return true; }
	virtual _bool TryGetSkillTargetPosition(_float3& OutPosition) const
	{
		return false;
	}
	_bool	Monster_Type(MONSTER_TYPE eType) { if (m_eMonType == eType)return true;  return false; }

protected:
	MONSTER_TYPE	m_eMonType{};

};

NS_END

