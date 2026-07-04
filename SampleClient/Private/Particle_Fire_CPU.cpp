#include "pch.h"
#include "Particle_Fire_CPU.h"
#include "Resources.h"
#include "GameInstance.h"

NS_USING(Client)


CParticle_Fire_CPU::CParticle_Fire_CPU() : CParticle_CPU()
{
  
   // CGameInstance::Get().GetGraphicDeviceContext()->PSSetShaderResources(9, 1, pTextureArray->GetSRV().GetAddressOf());
}

   

CParticle_Fire_CPU::CParticle_Fire_CPU(const CParticle_Fire_CPU& rhs)
    : CParticle_CPU(rhs)
{
}

CParticle_Fire_CPU::~CParticle_Fire_CPU()
{
}

HRESULT CParticle_Fire_CPU::Initialize(void* pArg)
{
    DESC desc{};
    desc.iMaxParticles = 50;
    desc.viBufferID = { "SAMPLE_CLIENT_PARTICLEBF", "VIBUF_ParticleQuad" };
    desc.textureID = { "SAMPLE_CLINET_TEXTURE", "TEX_FLARE" };
    desc.VSID = { "SAMPLE_CLIENT_SHADER", "VS_VTX_CPU_PARTICLE_TEX" };
    desc.PSID = { "SAMPLE_CLIENT_SHADER", "PS_VTX_CPU_PARTICLE_TEX" };
    desc.type = E::PARTICLE_TYPE::FIRE;

    return __super::Initialize(&desc);
}

void CParticle_Fire_CPU::UpdateBehavior(PARTICLE_CPU_DATA& p, E::_float fTimeDelta)
{
    // 부력 (하드코딩) - Spawn()으로 받은 초기 velocity 위에 계속 더해짐
    p.vVelocity.y += 4.f * fTimeDelta;
    p.vVelocity.x *= (1.f - 0.5f * fTimeDelta);
    p.vVelocity.z *= (1.f - 0.5f * fTimeDelta);

    p.vPosition.x += p.vVelocity.x * fTimeDelta;
    p.vPosition.y += p.vVelocity.y * fTimeDelta;
    p.vPosition.z += p.vVelocity.z * fTimeDelta;
    p.vColor = _float4((1.f), (0/255.f), (0/255.f), 1.f);
    p.vColor.w = 1.f - (p.fAge / p.fLifeTime);
     
}

UPtr<CParticle> CParticle_Fire_CPU::Create()
{
    return ToUPtr(new CParticle_Fire_CPU{});
}

