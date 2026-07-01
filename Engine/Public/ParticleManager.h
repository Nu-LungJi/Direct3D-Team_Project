#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CParticleManager final : public CEngineBase
{
private:
	CParticleManager();
	~CParticleManager();

public:
	void UpdateGUI();


public:
	void Update(_float fTimeDelta);
	HRESULT Render();

	void FrameStart(_float fTimeDelta);
	void FrameEnd(_float fTimeDelta);


public:
	static UPtr<CParticleManager> Create();

private:
	std::vector<PARTICLE> particles;
};

NS_END