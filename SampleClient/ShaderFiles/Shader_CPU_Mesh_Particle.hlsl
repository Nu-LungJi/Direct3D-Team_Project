#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"


#define MAX_LIGHT_COUNT     8
#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

struct VS_IN
{
	float3 vPosition : POSITION;
	float3 vNormal : NORMAL;
	float3 vTangent : TANGENT;
	float3 vBinormal : BINORMAL;
	float2 vTexcoord : TEXCOORD0;

    // Per-Instance - VTX_PARTICLE_INSTANCED_DATA와 바이트 레이아웃 일치.
    // "INSTANCE_" 접두사가 있어야 CResVertexShader::Load()의 리플렉션이
    // 이 필드들을 슬롯 1(인스턴스 버퍼)로 인식한다.
	float4 vWorld0 : INSTANCE_WORLD0;
	float4 vWorld1 : INSTANCE_WORLD1;
	float4 vWorld2 : INSTANCE_WORLD2;
	float4 vWorld3 : INSTANCE_WORLD3;
	float4 vColor : INSTANCE_COLOR0;
	float4 vInstEmissive : INSTANCE_EMISSIVE;
	float4 vInstEndEmissive : INSTANCE_EMISSIVE1;
	float4 vInstOriginalEmissive : INSTANCE_EMISSIVE2;
	float2 uvOffset : INSTANCE_UVOFFSET;
	float2 uvSize : INSTANCE_UVSIZE;
	float life : INSTANCE_LIFE; // 추가 
	float maxLife : INSTANCE_MAXLIFE; // 추가
	uint iBehaviorType : INSTANCE_BEHAVIORTYPE;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float3 vNormal : NORMAL0;
    float3 vTangent : TANGENT0;
    float3 vBinormal : BINORMAL0;
    float4 vEmissive : COLOR1;
    float4 vEndEmissive : COLOR2;
    float3 vWorldPos : TEXCOORD1;
    float life : TEXCOORD2;
    float maxLife : TEXCOORD3;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

	matrix matWorld = matrix(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);
    float4 vWorldPos = mul(float4(In.vPosition, 1.0f), matWorld);

    Out.vPosition = mul(vWorldPos, g_matViewProj);
    Out.vWorldPos = vWorldPos.xyz;
    Out.vTexcoord = In.vTexcoord;
    Out.vNormal = normalize(mul(In.vNormal, (float3x3) matWorld));
    Out.vTangent = normalize(mul(In.vTangent, (float3x3) matWorld));
    Out.vBinormal = normalize(mul(In.vBinormal, (float3x3) matWorld));
    Out.vColor = In.vColor;
    Out.vEmissive = In.vInstEmissive;
    Out.life = In.life; 
    Out.maxLife = In.maxLife;
    Out.vEndEmissive = In.vInstEndEmissive;
    return Out;
}

Texture2D   AlbedoMap    : register(t0);
Texture2D   NormalMap    : register(t1);
Texture2D   SMROMap      : register(t2);
Texture2D   EmissiveMap : register(t3);
Texture2D   DepthMap    : register(t4);
Texture2D   NoiseMap    : register(t5);
Texture2D   DistortionMap    : register(t6);
Texture2D  AnyTextureMap    : register(t7);

//SamplerState g_LinearSampler : register(s0);

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
};

PS_OUT PSMain(VS_OUT In)
{
    PS_OUT Out = (PS_OUT) 0;
    
    float DepthData = DepthMap.Sample(LinearWrap, In.vTexcoord).r;
    
    [branch]
    if (DepthData >= 1.0f)
        discard;
    
    float4 noise = NoiseMap.Sample(LinearWrap, In.vTexcoord);
    
    float ratio = 1.0f - (In.life / In.maxLife);

    if (noise.r < ratio) 
        discard;
    
    float3 DepthWorld = ReconstructWorldPos(In.vTexcoord, DepthData);
    
    float4 AlbedoTex = AlbedoMap.Sample(LinearWrap, In.vTexcoord) * float4(AlbedoColor, ObjectAlpha) * In.vColor;
    if (AlbedoTex.a == 0.0f)
        discard;

    float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

    float3 WorldNormal = Compute_WorldNormal(NormalMap, In.vTexcoord, In.vNormal, In.vTangent);
    WorldNormal = normalize(WorldNormal * NormalIntensity);

    float3 V = normalize(g_vCamPos - DepthWorld); //In.vWorldPos);
    float  NDV = max(dot(WorldNormal, V), 0.f);

    float3 SMRO = SMROMap.Sample(LinearWrap, In.vTexcoord).rgb;
    float fMetallic  = SMRO.r * MetallicIntensity;
    float fRoughness = SMRO.g * RoughnessIntensity;
    float fAmbient   = SMRO.b * AmbientIntensity;

    float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, fMetallic);

    float3 LightAccumulation = float3(0.f, 0.f, 0.f);

    [unroll(MAX_LIGHT_COUNT)]
    for (int i = 0; i < LightCount; ++i)
    {
        float3 L, Radiance;

        [branch]
        if (!Compute_DynamicLight(AffectedLight[i], DepthWorld, L, Radiance))
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

    // 인스턴스별 이미시브 + 오브젝트 이미시브 텍스처 둘 다 반영
    float3 texEmissive = EmissiveMap.Sample(LinearWrap, In.vTexcoord).rgb + EmissiveColor * EmissiveIntensity;
    texEmissive = pow(texEmissive, 2.2f);
    float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
    float3 instEmissive = lerpedEmissive.rgb * lerpedEmissive.a;
    
    float3 ConstantAmbient = Albedo * 0.05f * fAmbient;
    float3 FinalColor = ConstantAmbient + LightAccumulation + texEmissive + instEmissive;

    Out.vDiffuse = float4(FinalColor, AlbedoTex.a);
    return Out;
    
}
PS_OUT PS_SMOKE_MAIN(VS_OUT In)
{
	PS_OUT Out;
	float2 noiseUV = float2(In.vTexcoord.x * 5.f , In.vTexcoord.y * 0.12f);
	noiseUV.y += In.life * 0.05f;
		
	float2 warpUV = float2(In.vTexcoord.x * 3.f, In.vTexcoord.y * 0.35f);
	warpUV.y += In.life * 0.015f;

	float2 warp = NormalMap.Sample(LinearWrap, warpUV).rg * 2.f - 1.f;
	float noise = NoiseMap.Sample(LinearWrap, noiseUV + warp * 0.015f).r;

	
		//마스크는 픽셀을 얼마나 보여주는지에대한것	
	float heightMask = smoothstep(0.35f, 1.f, In.vTexcoord.y + (noise - 0.5f) * 0.45f);
	float softnoise = smoothstep(0.15f, 0.85f, noise);
	heightMask *= 1.f - smoothstep(0.85f, 1.f, In.vTexcoord.y);
	
	float t = saturate(In.life / In.maxLife);
	float lifeFade = smoothstep(0.f, 0.1f, t)*
	(1.f - smoothstep(0.5f, 1.f, t));
	float alpha = heightMask * In.vColor.a * lifeFade * lerp(0.08f, 1.f, softnoise);
	
	alpha = saturate(alpha);
	
	float2 distortion = warp * 0.01f * alpha;
	float3 glowColor = In.vColor.rgb * lerp(0.3f ,1.f, softnoise);

	Out.vDiffuse = float4(glowColor, alpha);
	
	return Out;
}
