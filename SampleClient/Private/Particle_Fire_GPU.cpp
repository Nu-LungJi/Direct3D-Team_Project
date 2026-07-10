#include "pch.h"
#include "Particle_Fire_GPU.h"
#include "Resources.h"
#include "GameInstance.h"

NS_USING(Client)


CParticle_Fire_GPU::CParticle_Fire_GPU() : CParticle_GPU()
{

    // CGameInstance::Get().GetGraphicDeviceContext()->PSSetShaderResources(9, 1, pTextureArray->GetSRV().GetAddressOf());
}




CParticle_Fire_GPU::~CParticle_Fire_GPU()
{
}

HRESULT CParticle_Fire_GPU::Initialize(void* pArg)
{
	char buf[128];
	sprintf_s(buf, "sizeof(CParticle_Fire_GPU) = %zu\n", sizeof(CParticle_Fire_GPU));
	OutputDebugStringA(buf);
    DESC desc{};
    desc.whatKind = MESHORTEXTURE::MESH;
    desc.iMaxParticles = 1000;
    desc.VSID = { "SAMPLE_CLIENT_SHADER", "VS_VTX_GPU_PARTICLE_MESH" };
    desc.PSID = { "SAMPLE_CLIENT_SHADER", "PS_VTX_GPU_PARTICLE_MESH" };

    desc.sGroupTag = "Rock1";
    desc.sResTag = "Static_Model_Resource";
	desc.iBehaviorType = 1;

    //desc.viBufferID = { "SAMPLE_CLIENT_PARTICLEBF", "VIBUF_ParticleQuad" };
    //desc.textureID = { "SAMPLE_CLINET_TEXTURE", "TEX_FLARE" };
    //desc.VSID = { "SAMPLE_CLIENT_SHADER", "VS_VTX_GPU_PARTICLE_TEX" };
    //desc.PSID = { "SAMPLE_CLIENT_SHADER", "PS_VTX_GPU_PARTICLE_TEX" };
    desc.type = E::PARTICLE_TYPE::FIRE_GPU;

    return __super::Initialize(&desc);
}


UPtr<CParticle> CParticle_Fire_GPU::Create()
{
	
	
    return ToUPtr(new CParticle_Fire_GPU{});
}

