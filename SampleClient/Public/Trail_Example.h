#pragma once
#include "Client_Defines.h"
#include "Trail_CPU.h"

NS_BEGIN(Client)

class CTrail_Example final : public CTrail_CPU
{
public:
    DECLARE_DERIVED_TYPE(CTrail_Example, CTrail_CPU)

private:
    explicit CTrail_Example();
    CTrail_Example(const CTrail_Example& rhs);
    virtual ~CTrail_Example();
    virtual HRESULT Initialize(void* pArg) override;

public:
    static UPtr<CParticle> Create();
};

NS_END