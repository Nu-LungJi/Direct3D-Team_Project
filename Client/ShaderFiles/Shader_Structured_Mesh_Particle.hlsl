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
Texture2D DistortionMap : register(t6);
Texture2D g_BackgroundTex : register(t7);

Texture2D AnyTextureMap : register(t8);



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
	float4 vScreenPos : TEXCOORD4;
};

VS_OUT VSMain(VS_IN In, uint instID : SV_InstanceID)
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
	Out.vScreenPos = Out.vPosition;
    return Out;
}

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
};


PS_OUT PSMain(VS_OUT In)
{
    PS_OUT Out = (PS_OUT) 0;

    float4 AlbedoTex = AlbedoMap.Sample(LinearWrap, In.vTexcoord) * In.vColor;
    if (AlbedoTex.a < 0.05f)
        discard;
    float4 noise = NoiseMap.Sample(LinearWrap, In.vTexcoord);
    
    float ratio = 1.0f - (In.life / In.maxLife);

    if (noise.r < ratio) 
        discard;
    //float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

  


    // 인스턴스(파티클)별 이미시브 + 오브젝트 이미시브 텍스처 둘 다 반영
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, saturate(ratio * 1.5f));
    float3 instEmissive = lerpedEmissive.rgb * lerpedEmissive.a;

    float3 FinalColor = AlbedoTex.rgb + instEmissive;
	
    Out.vDiffuse = float4(FinalColor, AlbedoTex.a);
    return Out;
}

PS_OUT PSMaceSphere(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ratio = saturate(1.0f - In.life / max(In.maxLife, 0.0001f));
	float fade = saturate(1.0f - ratio);

	float2 noiseUV1 = In.vTexcoord + float2(g_fTime * 0.05f, -g_fTime * 0.08f);
	float2 noiseUV2 = In.vTexcoord * 1.7f + float2(-g_fTime * 0.07f, g_fTime * 0.04f);

	float noise1 = NoiseMap.Sample(LinearWrap, noiseUV1).r;
	float noise2 = NoiseMap.Sample(LinearWrap, noiseUV2).g;
	float surfaceNoise = saturate(noise1 * 0.7f + noise2 * 0.5f);

	float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
	screenUV = screenUV * float2(0.5f, -0.5f) + 0.5f;

	float2 distortion = DistortionMap.Sample(LinearWrap, noiseUV1).rg * 2.0f - 1.0f;
	float distortionStrength = 0.08f * fade;
	distortion *= distortionStrength * lerp(0.4f, 1.0f, surfaceNoise);

	float3 distortedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion).rgb;

	float4 albedoTex = AlbedoMap.Sample(LinearWrap, In.vTexcoord);
	albedoTex *= float4(AlbedoColor, ObjectAlpha) * In.vColor;

	float alphaMask = albedoTex.a * smoothstep(0.15f, 0.65f, surfaceNoise);
	clip(alphaMask - 0.01f);

	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float3 instEmissive = lerpedEmissive.rgb * lerpedEmissive.a;
	float3 albedo = pow(max(albedoTex.rgb, 0.0f), 2.2f);

	float3 surfaceColor = albedo + instEmissive * surfaceNoise;
	float surfaceOpacity = saturate(alphaMask * 0.7f * fade);
	float3 finalColor = lerp(distortedBackground, surfaceColor, surfaceOpacity);

	Out.vDiffuse = float4(finalColor, 1.0f);
	return Out;
}
