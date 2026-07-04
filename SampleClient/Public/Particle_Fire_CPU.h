#pragma once
#include "Client_Defines.h"
#include "Particle_CPU.h"

NS_BEGIN(Client)

class CParticle_Fire_CPU final : public CParticle_CPU
{
public:
    DECLARE_DERIVED_TYPE(CParticle_Fire_CPU, CParticle_CPU)

private:
    explicit CParticle_Fire_CPU();
    CParticle_Fire_CPU(const CParticle_Fire_CPU& rhs);
    virtual ~CParticle_Fire_CPU();
    virtual HRESULT Initialize(void* pArg) override;
protected:
    // 초기 velocity/life는 Spawn()에서 외부로부터 받으므로 여기선 지속되는 힘만 처리
    virtual void UpdateBehavior(PARTICLE_CPU_DATA& p, E::_float fTimeDelta) override;

public:
    static UPtr<CParticle> Create();
};

NS_END