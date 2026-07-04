#pragma once
#include "Client_Defines.h"
#include "Beam_CPU.h"

NS_BEGIN(Client)

class CParticle_Ribbon final : public CBeam_CPU
{
public:
    DECLARE_DERIVED_TYPE(CParticle_Ribbon, CBeam_CPU)

private:
    explicit CParticle_Ribbon();
    CParticle_Ribbon(const CParticle_Ribbon& rhs);
    virtual ~CParticle_Ribbon();
    virtual HRESULT Initialize(void* pArg) override;

public:
    static UPtr<CParticle> Create();
};

NS_END