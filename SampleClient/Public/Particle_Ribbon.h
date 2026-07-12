#pragma once
#include "Client_Defines.h"
#include "Beam_CPU.h"

NS_BEGIN(Client)

class CParticle_Ribbon final 
{
public:


private:
    explicit CParticle_Ribbon();
    virtual ~CParticle_Ribbon();
    virtual HRESULT Initialize(void* pArg);

public:
    static UPtr<CParticle> Create();
};

NS_END
