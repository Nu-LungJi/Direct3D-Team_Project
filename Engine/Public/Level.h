#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CLevel : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CLevel, CEngineBase)
	static constexpr uint32_t INVALID_LEVEL_ID = static_cast<uint32_t>(-1);

protected:
	explicit CLevel(uint32_t iLevelID = INVALID_LEVEL_ID);
	virtual ~CLevel();

public:
	uint32_t GetLevelID() const { return m_iLevelID; }

	virtual HRESULT Initialize();
	virtual void Update(_float fTimeDelta);
	virtual HRESULT Render();
	virtual void FrameStart(_float fTimeDelta);
	virtual void FrameEnd(_float fTimeDelta);
	virtual void UpdateGUI();

private:
	const uint32_t m_iLevelID{ INVALID_LEVEL_ID };
};

NS_END
