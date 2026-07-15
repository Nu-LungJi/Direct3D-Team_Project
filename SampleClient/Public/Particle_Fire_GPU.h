#pragma once
#include "Client_Defines.h"
#include "Particle_GPU.h"

NS_BEGIN(Client)

class CParticle_Fire_GPU final 
{
public:

private:
    explicit CParticle_Fire_GPU();
    virtual ~CParticle_Fire_GPU();
    virtual HRESULT Initialize(void* pArg);
    HRESULT Initialize();
public:
    static UPtr<CParticle> Create();
};

NS_END
