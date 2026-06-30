#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class ENGINE_DLL CTimer final
{
public:
	_float Get_CurrTime() const { return m_currTime; }
	void Set_CurrTime(float time) { m_currTime = time; }

	_float Get_GoalTime() const { return m_goalTime; }
	void Set_GoalTime(float time) { m_goalTime = time; m_bHasGoalTime = true; }

	_bool Get_Finished() const { return m_bFinished; }
	_bool Get_JustFinished() const { return m_bJustFinished; }

	_bool Get_Resetted() const { return m_bResetted; }

	void Reset() { m_currTime = 0.f; m_bFinished = false; m_bJustFinished = false; m_bResetted = true; }
	void Reset(float fResetTime) { m_currTime = fResetTime; m_bFinished = false; m_bJustFinished = false;  m_bResetted = true;
	}

public:
	void AppendCurrTime(float time)
	{
		if (m_bResetted)
		{
			m_bResetted = false;
		}
		if (m_bHasGoalTime)
		{
			float cachedCurrTime = m_currTime;
			m_currTime += time;

			if (cachedCurrTime < m_goalTime && m_currTime >= m_goalTime)
			{
				m_bJustFinished = true;
			}
			else
			{
				m_bJustFinished = false;
			}

			if (m_currTime >= m_goalTime)
			{
				m_bFinished = true;
			}
		}
		else
		{
			m_currTime += time;
		}
	}

private:
	_float m_currTime{};
	_float m_goalTime{};

	_bool m_bJustFinished{ false };
	_bool m_bFinished{ false };
	_bool m_bHasGoalTime{ false };
	_bool m_bResetted{ true };
};

NS_END
