//#include "pch.h"
//#include "Particle_Ribbon.h"
//#include "Resources.h"
//#include "GameInstance.h"
//
//NS_USING(Client)
//
//
//CParticle_Ribbon::CParticle_Ribbon() : CBeam_CPU()
//{
//
//    // CGameInstance::Get().GetGraphicDeviceContext()->PSSetShaderResources(9, 1, pTextureArray->GetSRV().GetAddressOf());
//}
//
//
//
//
//
//CParticle_Ribbon::~CParticle_Ribbon()
//{
//}
//
//HRESULT CParticle_Ribbon::Initialize(void* pArg)
//{
//    DESC desc{};
//
//  //  desc.viBufferID = { "SAMPLE_CLIENT_PARTICLEBF", "VIBUF_ParticleQuad" };
//    desc.textureID = { "SAMPLE_CLINET_TEXTURE", "TEX_RIBBON" };
//    desc.VSID = { "SAMPLE_CLIENT_SHADER", "VS_VTX_RIBBON_PARTICLE_TEX" };
//    desc.PSID = { "SAMPLE_CLIENT_SHADER", "PS_VTX_RIBBON_PARTICLE_TEX" };
//    desc.type = E::PARTICLE_TYPE::RIBBON;
//
//    return __super::Initialize(&desc);
//}
//
//
//
//UPtr<CParticle> CParticle_Ribbon::Create()
//{
//    return ToUPtr(new CParticle_Ribbon{});
//}
//
