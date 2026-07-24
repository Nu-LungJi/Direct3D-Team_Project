#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

cbuffer CB_PER_PARTICLE : register(b5)		// 전역으로 이동 예정
{
	float	g_fTimeDelta;
	uint	g_iNumInstances;
	uint	g_iFlipbookRows;
	uint	g_iFlipbookColumns;
	uint	g_iTotalFrames;
	float	g_fTime;
	float2	g_fPadding;
};

StructuredBuffer<ParticleData> g_RenderBuffer : register(t4);

//픽셀 쉐이더용
Texture2D AlbedoMap		: register(t0);
Texture2D NormalMap		: register(t1);
Texture2D SMROMap		: register(t2);
Texture2D EmissiveMap	: register(t3);
Texture2D NoiseMap		: register(t5);
Texture2D g_BackgroundTex : register(t7);

const static float SliceCount = 64.f;

struct VS_IN
{
    // Per-Vertex - 쿼드 메쉬 로컬 좌표 (-0.5~0.5), UV
	float3 vPosition : POSITION;
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

	float4 vColor : TEXCOORD1;
	float4 vEmissive : TEXCOORD2;
	float4 vEndEmissive : TEXCOORD3;

	uint iBehaviorType : TEXCOORD4;
	float4 vScreenPos : TEXCOORD5;
	float3 vNormal : TEXCOORD6;
	float3 vTangent : TEXCOORD7;
	float3 vWorldPos : TEXCOORD8;
	float life : TEXCOORD9;
	float maxLife : TEXCOORD10;
};

float4 PSMain(VS_OUT In) : SV_TARGET
{
	float2	UVScale  = float2(1.f, 1.f);
	float2  UVScroll = float2(0.f, 0.f);
	
	float2 UVNoise = In.vTexcoord * float2(1.f, 1.f);//-float2(0.f, In.maxLife * 2.f);
	UVNoise += In.life * 0.5f;
	float4	PackColor = AlbedoMap.Sample(LinearWrap, UVNoise);
	
	float	DistortionNoiseValue = AlbedoMap.Sample(LinearWrap, UVNoise).r;
	
	float	MainCore = PackColor.r;
	float	Branch	 = PackColor.g;
	float	CombinedShape = saturate((MainCore + Branch * 0.7f) * (DistortionNoiseValue * 0.5f + 0.75f));
	
	float	Progress = 1.f - (In.life / In.maxLife);
	
	if (DistortionNoiseValue < Progress) 	discard;
	
	float	AppearDissolve = smoothstep(0.f, 0.1f, Progress);
	float	DisappearDissolve = 1.f - smoothstep(0.8f, 1.f, Progress);
	
	float	DissolveThreshold = AppearDissolve * DisappearDissolve;
	float	DissolveMap = CombinedShape * (DistortionNoiseValue * 0.5f + 0.5f);
	
	float	DissolveMask = smoothstep(1.0f - DissolveThreshold - 0.15f, 1.0f - DissolveThreshold, DissolveMap);
	CombinedShape *= DissolveMask;
	
	float	OuterMask = smoothstep(0.10f, 0.50f, CombinedShape);
	float	CoreMask = smoothstep(0.55f, 0.90f, CombinedShape);
	
	float3	BlueGlow = float3(0.1f, 0.5f, 1.2f);
	float3	CoreGlow = float3(1.2f, 1.2f, 1.2f);
	
	float	GlowIntensity = 6.f;
	float	CoreIntensity = 2.f;
	
	float EdgeGlow = saturate(OuterMask - CoreMask);
	float3 GlowEmissive = EdgeGlow * BlueGlow * GlowIntensity;
	float3 CoreEmissive = CoreMask * CoreGlow * CoreIntensity;
	
	float FadeIn = smoothstep(0.f, 0.1f, Progress);
	float FadeOut = 1.f - smoothstep(0.8f, 1.f, Progress);
	float LifeMask = FadeIn * FadeOut;
	
	//float Mask = saturate(max(OuterMask, CoreMask));
	float3 Emissive = (OuterMask + CoreMask) * LifeMask;
	float  Alpha = OuterMask * LifeMask;
	
	return float4(In.vTexcoord.yyy, Alpha);
}

float4 PSMain_Extra(VS_OUT In) : SV_TARGET
{
	float4 NoiseTex = AlbedoMap.Sample(LinearWrap, In.vTexcoord);
	NoiseTex = pow(NoiseTex, 2.2f);

	float2 noiseUV0 = In.vTexcoord + float2(0.13, 1.5f) * In.maxLife;

	float2 noiseUV1 = In.vTexcoord * 1.73 - float2(0.21, 1.5f * 0.61) * In.maxLife;
	
	float Progress = 1.f - (In.life / In.maxLife);
	
	float4 noiseSample0 = AlbedoMap.Sample(LinearWrap, noiseUV0);
	float4 noiseSample1 = AlbedoMap.Sample(LinearWrap, noiseUV1);
	
	float noise0 = noiseSample0.r;
	float noise1 = noiseSample1.g;

	float NoiseMask = saturate(noise0 * noise1 * 2.0);

	float3 TexCoord3D;
	TexCoord3D.xy = In.vWorldPos.xy * 0.01f;
	TexCoord3D.z  = In.maxLife * 0.5f; 
	float3 VolumeNoise = SampleTexture3D(SMROMap, TexCoord3D);
	
	float CombinedNoise = saturate(NoiseMask * 0.65f + VolumeNoise * 0.35f);
	float OuterMask = smoothstep(0.15f, 0.60f, CombinedNoise);
	float CoreMask  = smoothstep(0.60f, 0.95f, CombinedNoise);
	
	float3 OuterDiffuse = float3(0.05f, 0.45f, 1.f);
	float3 CoreDiffuse = float3(1.f, 0.95f, 0.9f);
	
	float GlowIntensity = 5.f;
	float CoreIntensity = 10.f;
	
	float EdgeMask = saturate(OuterMask - CoreMask);
	float3 GlowEmissive = EdgeMask * GlowIntensity * OuterDiffuse;
	float3 CoreEmissive = CoreMask * CoreIntensity * CoreDiffuse;
	
	float FadeIn = smoothstep(0.f, 0.06f, Progress);
	float FadeOut = 1.f - smoothstep(0.35f, 1.f, Progress);
	float LifeMask = FadeIn * FadeOut;
	
	//float Mask = saturate(max(OuterMask, CoreMask));
	float3 Emissive = (OuterMask + CoreMask) * 1.f * LifeMask;
	float Alpha = OuterMask * 1.f * LifeMask;
	
	return float4(Emissive, Alpha);
}
