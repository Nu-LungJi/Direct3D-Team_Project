#pragma once
#include "Client_Defines.h"
#include "Particle_CPU.h"

NS_BEGIN(Client)

class CParticle_Fire_CPU final
{
public:

private:
    explicit CParticle_Fire_CPU();
    virtual ~CParticle_Fire_CPU();
    virtual HRESULT Initialize(void* pArg) ;
private:
    // 초기 velocity/life는 Spawn()에서 외부로부터 받으므로 여기선 지속되는 힘만 처리


public:
    static UPtr<CParticle> Create();
};

NS_END
