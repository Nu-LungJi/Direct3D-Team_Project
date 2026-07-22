#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"
#define MAX_LIGHT_COUNT     8
#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

cbuffer CB_PER_PARTICLE : register(b11)
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

//버텍스 쉐이더용 

Texture2D hdrPoisitonMap : register(t10);
Texture2D hdrNormalMap : register(t11);


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
    float4 vEmissive : EMISSIVE;
    float3 vWorldPos : TEXCOORD1; // 추가: 라이팅 계산에 필요
    float life : TEXCOORD2;
    float maxLife : TEXCOORD3;
};

VS_OUT VSMain(VS_IN In, uint instID : SV_InstanceID, uint vertID : SV_VertexID)
{
    VS_OUT Out = (VS_OUT) 0;
    ParticleData p = g_RenderBuffer[instID];
    float2 finalUV = In.vTexcoord;
	float3 scale = p.alive ? p.size : float3(0.0f, 0.0f, 0.0f);

    if (g_iTotalFrames > 1 && g_iFlipbookColumns > 0 && g_iFlipbookRows > 0)
    {
        uint frame = min(p.frameIndex, g_iTotalFrames - 1);
        uint col = frame % g_iFlipbookColumns;
        uint row = frame / g_iFlipbookColumns;
        float2 uvSize = float2(1.0f / g_iFlipbookColumns, 1.0f / g_iFlipbookRows);
        float2 uvOffset = float2(col, row) * uvSize;
        finalUV = uvOffset + In.vTexcoord * uvSize;
    }
    Out.vTexcoord = finalUV;

// ---- VAT (Ripple: UV0 기반) ----------------------------------------------------
    //uint vatWidth, vatHeight;
    //hdrPoisitonMap.GetDimensions(vatWidth, vatHeight);
    //
    //float ratio = saturate(1.0f - (p.life / p.maxLife));
    //float u = In.vTexcoord.x;
    //
    //// 프레임(행) 두 개를 구해서 수동 보간
    //float frameF = ratio * (vatHeight - 1);
    //uint row0 = (uint) floor(frameF);
    //uint row1 = min(row0 + 1, vatHeight - 1);
    //float blend = frac(frameF);
    //
    //float v0 = (row0 + 0.5f) / vatHeight;
    //float v1 = (row1 + 0.5f) / vatHeight;
    //
    //float3 pos0 = hdrPoisitonMap.SampleLevel(PointWrap, float2(u, v0), 0).xyz;
    //float3 pos1 = hdrPoisitonMap.SampleLevel(PointWrap, float2(u, v1), 0).xyz;
    //float3 posOffset = lerp(pos0, pos1, blend);
    //float3 vLocalPos = In.vPosition + posOffset;
    //
    //float3 n0 = hdrNormalMap.SampleLevel(PointWrap, float2(u, v0), 0).xyz;
    //float3 n1 = hdrNormalMap.SampleLevel(PointWrap, float2(u, v1), 0).xyz;
    //float3 vLocalNormal = normalize(lerp(n0, n1, blend) * 2.0f - 1.0f);

    //(splatter)
    
    uint vatWidth, vatHeight;
    hdrPoisitonMap.GetDimensions(vatWidth, vatHeight);
    float ratio = saturate(1.0f - (p.life / p.maxLife));
    float u = (float(vertID) + 0.5f) / float(vatWidth);

    // 프레임(행) 두 개 구해서 보간
    float frameF = ratio * (vatHeight - 1);
    uint row0 = (uint) floor(frameF);
    uint row1 = min(row0 + 1, vatHeight - 1);
    float blend = frac(frameF);
    float v0 = (row0 + 0.5f) / vatHeight;
    float v1 = (row1 + 0.5f) / vatHeight;

    float3 pos0 = hdrPoisitonMap.SampleLevel(PointClamp, float2(u, v0), 0).xyz;
    float3 pos1 = hdrPoisitonMap.SampleLevel(PointClamp, float2(u, v1), 0).xyz; 
    float3 posOffset = lerp(pos0, pos1, blend);
    float3 vLocalPos = In.vPosition + posOffset;

    float3 n0 = hdrNormalMap.SampleLevel(PointClamp, float2(u, v0), 0).xyz;
    float3 n1 = hdrNormalMap.SampleLevel(PointClamp, float2(u, v1), 0).xyz;
    float3 vLocalNormal = normalize(lerp(n0, n1, blend) * 2.0f - 1.0f); // VN이 8비트 PNG라 *2-1 디코드 필요
// 스케일 적용
    vLocalPos *= scale;

// 파티클 회전
    float3 rotatedLocal = RotateXYZ(vLocalPos, p.rotation);

// 월드 위치
    float3 worldPos = rotatedLocal + p.position;

    Out.vPosition = mul(float4(worldPos, 1.0f), g_matViewProj);
    Out.vWorldPos = worldPos;

// 노멀/탄젠트/바이노멀 회전
    Out.vNormal = normalize(RotateXYZ(vLocalNormal, p.rotation));
    Out.vTangent = normalize(RotateXYZ(In.vTangent, p.rotation));
    Out.vBinormal = normalize(RotateXYZ(In.vBinormal, p.rotation));
    Out.vColor = p.alive ? p.color : float4(p.color.rgb, 0.0f);
    Out.vEmissive = p.emissive;
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

    // AlbedoMap 샘플링 제거, 재질 상수 + 파티클 컬러만 사용
    float4 AlbedoTex = float4(AlbedoColor, ObjectAlpha) * In.vColor;
    if (AlbedoTex.a < 0.05f)
        discard;

    float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

    // NormalMap 없이 VAT에서 이미 계산된 월드 노말 그대로 사용
    float3 WorldNormal = normalize(In.vNormal);

    float3 V = normalize(g_vCamPos - In.vWorldPos);
    float NDV = max(dot(WorldNormal, V), 0.f);

    // SMROMap 없이 고정값(원하는 느낌으로 튜닝)
    float fMetallic = MetallicIntensity; // 예: 0
    float fRoughness = max(RoughnessIntensity, 0.05f); // 0 방지
    float fAmbient = AmbientIntensity;

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

    float3 instEmissive = In.vEmissive.rgb * In.vEmissive.a;
    float3 ConstantAmbient = Albedo * 0.05f * fAmbient;
    float3 FinalColor = ConstantAmbient + LightAccumulation + instEmissive;

    Out.vDiffuse = float4(FinalColor, AlbedoTex.a);
    return Out;
}
