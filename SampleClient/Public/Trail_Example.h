#pragma once
#include "Client_Defines.h"
#include "Trail_CPU.h"

NS_BEGIN(Client)

class CTrail_Example final
{
public:


private:
    explicit CTrail_Example();
    virtual ~CTrail_Example();
    virtual HRESULT Initialize(void* pArg) ;

public:
    static UPtr<CParticle> Create();
};

NS_END
