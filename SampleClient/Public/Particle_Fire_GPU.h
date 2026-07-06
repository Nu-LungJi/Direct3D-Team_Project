#pragma once
#include "Client_Defines.h"
#include "Particle_GPU.h"

NS_BEGIN(Client)

class CParticle_Fire_GPU final : public CParticle_GPU
{
public:
    DECLARE_DERIVED_TYPE(CParticle_Fire_GPU, CParticle_GPU)

private:
    explicit CParticle_Fire_GPU();
    virtual ~CParticle_Fire_GPU();
    virtual HRESULT Initialize(void* pArg) override;
public:
    static UPtr<CParticle> Create();
};

NS_END