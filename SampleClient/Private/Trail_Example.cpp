//#include "pch.h"
//#include "Trail_Example.h"
//#include "Resources.h"
//#include "GameInstance.h"
//
//NS_USING(Client)
//
//
//CTrail_Example::CTrail_Example() : CTrail_CPU()
//{
//
//    // CGameInstance::Get().GetGraphicDeviceContext()->PSSetShaderResources(9, 1, pTextureArray->GetSRV().GetAddressOf());
//}
//
//
//
//
//
//CTrail_Example::~CTrail_Example()
//{
//}
//
//HRESULT CTrail_Example::Initialize(void* pArg)
//{
//    DESC desc{};
//    // desc.viBufferID = { "SAMPLE_CLIENT_PARTICLEBF", "VIBUF_ParticleQuad" };
//    desc.textureID = { "SAMPLE_CLINET_TEXTURE", "TEX_TRAIL" };
//    desc.VSID = { "SAMPLE_CLIENT_SHADER", "VS_VTX_Trail_TEX" };
//    desc.PSID = { "SAMPLE_CLIENT_SHADER", "PS_VTX_Trail_TEX" };
//    desc.type = E::PARTICLE_TYPE::TRAIL;
//
//
//    return __super::Initialize(&desc);
//}
//
//
//
//UPtr<CParticle> CTrail_Example::Create()
//{
//    return ToUPtr(new CTrail_Example{});
//}
//
