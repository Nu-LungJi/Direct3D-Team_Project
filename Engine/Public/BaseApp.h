#pragma once
#include "Engine_Defines.h"
#include "Timer.h"
NS_BEGIN(Engine)
class CLevel;
class ENGINE_DLL CBaseApp : public CEngineBase
{
protected:
	CBaseApp();
	~CBaseApp() override;

public:
	HRESULT Loop();

protected:
	void UpdateGUI();

protected:
	virtual void FixedUpdate(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual HRESULT Render(_float fInterpolation);
	virtual void FrameStart(_float fTimeDelta);
	virtual void FrameEnd(_float fTimeDelta);

protected:
	HRESULT Initialize(const ENGINE_DESC& engineDesc);
	HRESULT StartLevel(UPtr<CLevel> pStartLevel);

protected:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

protected:
	CTimer m_UpdateTimer{};
	CTimer m_MeasureTimer{};

	uint32_t m_iMeasureUpdateCnt{};
	uint32_t m_iMeasureUpdateCntPerSec{};
};

NS_END