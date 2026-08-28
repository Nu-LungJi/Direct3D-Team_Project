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
	_bool	Monster_Type(MONSTER_TYPE eType) { if (m_eMonType == eType)return true;  return false; }

protected:
	MONSTER_TYPE	m_eMonType{};

};

NS_END

