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
	float3 vPosition : POSITION;
	float3 vNormal	 : NORMAL;
	float3 vTangent  : TANGENT;
	float3 vBinormal : BINORMAL;
	float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
	float4 vPosition : SV_POSITION;
	float2 vTexcoord : TEXCOORD0;
	float4 vColor	 : COLOR0;
	float3 vNormal	 : NORMAL0;
	float3 vTangent  : TANGENT0;
	float3 vBinormal : BINORMAL0;
	float4 vEmissive : EMISSIVE0;
	float4 vEndEmissive : EMISSIVE1;
	float3 vWorldPos : TEXCOORD1;
	float life		 : TEXCOORD2;
	float maxLife	 : TEXCOORD3;
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

float SampleTextureSlice(Texture2D _Texture, float2 _TexCoord, float _Slice)
{
	_TexCoord = frac(_TexCoord);
	float2 Tex3DTexCoord;
	Tex3DTexCoord.x = _TexCoord.x;
	Tex3DTexCoord.y = (_TexCoord.y + _Slice) / SliceCount;
	
	return _Texture.Sample(LinearWrap, Tex3DTexCoord).r;
}

float SampleTexture3D(Texture2D _Texture, float3 _TexCoord3D)
{
	_TexCoord3D = frac(_TexCoord3D);
	
	float Z = _TexCoord3D.z * (SliceCount - 1.f);
	float Slice0 = floor(Z);
	float Slice1 = min(Slice0 + 1.f, SliceCount - 1.f);
	
	float BlendValue = frac(Z);
	
	float Value0 = SampleTextureSlice(_Texture, _TexCoord3D.xy, Slice0);
	float Value1 = SampleTextureSlice(_Texture, _TexCoord3D.xy, Slice1);

	return lerp(Value0, Value1, BlendValue);
}
float Hash(float value)
{
	return frac(sin(value * 12.9898) * 43758.5453);
}

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
