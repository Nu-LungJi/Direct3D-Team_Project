#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"
#define MAX_LIGHT_COUNT     8
#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

cbuffer CB_PER_PARTICLE : register(b5)
{
    float g_fTimeDelta;
    uint g_iNumInstances;
    uint g_iFlipbookRows;
    uint g_iFlipbookColumns;
    uint g_iTotalFrames;
    float g_fTime;
    float2 g_fPadding;
};

StructuredBuffer<ParticleData> g_RenderBuffer : register(t4);

//픽셀 쉐이더용
Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D SMROMap : register(t2);
Texture2D EmissiveMap : register(t3);
Texture2D NoiseMap : register(t5);



struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;

};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float3 vNormal : NORMAL0;
    float3 vTangent : TANGENT0;
    float3 vBinormal : BINORMAL0;
    float4 vEmissive : EMISSIVE0;
    float4 vEndEmissive : EMISSIVE1;
    float3 vWorldPos : TEXCOORD1; // 추가: 라이팅 계산에 필요
    float life : TEXCOORD2;
    float maxLife : TEXCOORD3;
};

VS_OUT VSMain(VS_IN In, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;
    ParticleData p = g_RenderBuffer[instID];
    float2 finalUV = In.vTexcoord;
    float scale = p.alive ? p.size : 0.0f;
    
    if (g_iTotalFrames > 1 && g_iFlipbookColumns > 0 && g_iFlipbookRows > 0)
    {
        uint frame = min(p.frameIndex, g_iTotalFrames - 1);
        uint col = frame % g_iFlipbookColumns;
        uint row = frame / g_iFlipbookColumns;
        float2 uvSize = float2(1.0f / g_iFlipbookColumns, 1.0f / g_iFlipbookRows);
        float2 uvOffset = float2(col, row) * uvSize;

        finalUV = uvOffset + In.vTexcoord * uvSize; // baseUV 대신 실제 메쉬 UV 사용
    }

    Out.vTexcoord = finalUV;
    

    float3 localPos = In.vPosition * scale; 
    float3 rotatedLocal = RotateXYZ(localPos, p.rotation); 
    float3 vWorldPos = rotatedLocal + p.position;


    Out.vPosition = mul(float4(vWorldPos, 1.0f), g_matViewProj);
    Out.vWorldPos = vWorldPos;
    //Out.vTexcoord = In.vTexcoord;
    Out.vNormal = In.vNormal;
    Out.vTangent = In.vTangent;
    Out.vBinormal = In.vBinormal;
    Out.vColor = p.alive ? p.color : float4(p.color.rgb, 0.0f);
    Out.vEmissive = p.emissive;
    Out.vEndEmissive = p.endEmissive;
    Out.life = p.life;
    Out.maxLife = p.maxLife;
    
    return Out;
}

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
};


PS_OUT PSMain(VS_OUT In)
{
    PS_OUT Out = (PS_OUT) 0;
    float scrollSpeed = 0.2f; // 흐르는 속도, 필요하면 파라미터로 빼서 조절
    float2 scrolledUV = In.vTexcoord + float2(scrollSpeed , 0.0f);
    
    
    float4 AlbedoTex = AlbedoMap.Sample(LinearWrap, scrolledUV) * float4(AlbedoColor, ObjectAlpha) * In.vColor;
    if (AlbedoTex.a < 0.05f)
        discard;
    float ratio = 1.0f - (In.life / In.maxLife);
    float4 noise = NoiseMap.Sample(LinearWrap, In.vTexcoord);
    float lengthMask = In.vTexcoord.y;
	
	float bandWidth = 0.35f;
// ratio(0~1)를 -bandWidth ~ (1+bandWidth) 범위로 확장해서 매핑
	float leadingEdge = lerp(-bandWidth, 1.0f + bandWidth, ratio);
	float trailingEdge = leadingEdge - bandWidth;

	if (lengthMask > leadingEdge || lengthMask < trailingEdge)
		discard;
// 길이 그라디언트 위주 + 노이즈로 가장자리에 자연스러운 디테일만 살짝
    //float revealValue = saturate(lengthMask * 0.7f + noise.r * 0.3f);

	// 다 안보였다가 보이게 됨
	//if (lengthMask > ratio)
	//	discard;
	//
	//if (lengthMask < mask2)
	//	discard;
    //if (noise.r > ratio) 
    //    discard;
    float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

    float3 WorldNormal = Compute_WorldNormal(NormalMap, In.vTexcoord, In.vNormal, In.vTangent);
    WorldNormal = normalize(WorldNormal * NormalIntensity);

    float3 V = normalize(g_vCamPos - In.vWorldPos);
    float NDV = max(dot(WorldNormal, V), 0.f);

    float3 SMRO = SMROMap.Sample(LinearWrap, In.vTexcoord).rgb;
    float fMetallic = SMRO.r * MetallicIntensity;
    float fRoughness = SMRO.g * RoughnessIntensity;
    float fAmbient = SMRO.b * AmbientIntensity;

    float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, fMetallic);

    float3 LightAccumulation = float3(0.f, 0.f, 0.f);

    [unroll(MAX_LIGHT_COUNT)]
    for (int i = 0; i < LightCount; ++i)
    {
        float3 L, Radiance;

        [branch]
        if (!Compute_DynamicLight(AffectedLight[i], In.vWorldPos, L, Radiance))
            continue;

        float RawNDL = dot(WorldNormal, L);

        [branch]
        if (RawNDL > 0.f)
        {
            float NDL = clamp(RawNDL, 0.f, 1.f);

            float3 H = normalize(V + L);
            float D = DistributionGGX(WorldNormal, H, fRoughness);
            float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);
            float V_Spec = VisibilitySmithJointGGX(NDV, NDL, fRoughness);

            float3 Specular = D * F * V_Spec * SpecularIntensity;

            float3 kS = F;
            float3 kD = (1.0 - kS) * (1.0 - fMetallic);
            float3 Diffuse = kD * Albedo / PI;

            LightAccumulation += (Diffuse + Specular) * Radiance * NDL;
        }
    }

    // 인스턴스(파티클)별 이미시브 + 오브젝트 이미시브 텍스처 둘 다 반영
    float3 texEmissive = EmissiveMap.Sample(LinearWrap, In.vTexcoord).rgb + EmissiveColor * EmissiveIntensity;
    texEmissive = pow(texEmissive, 2.2f);
    float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
    float3 instEmissive = lerpedEmissive.rgb * lerpedEmissive.a;

    float3 ConstantAmbient = Albedo * 0.05f * fAmbient;
    float3 FinalColor = ConstantAmbient + LightAccumulation + texEmissive + instEmissive;

    
    
    Out.vDiffuse = float4(FinalColor, AlbedoTex.a);
    return Out;
}
