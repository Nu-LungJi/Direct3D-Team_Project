#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

const static float DistortionSTR = 0.3f;
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
Texture2D g_DiffuseTexture : register(t1);
Texture2D g_NormalTexture : register(t2);
Texture2D g_DistortionTexture : register(t3);
Texture2D g_NoiseTexture : register(t4);
Texture2D g_AnyTexture : register(t5);
Texture2D g_BackgroundTex : register(t7);
VS_OUT VSMain(VS_IN In)
{
	VS_OUT Out = (VS_OUT) 0;

	float4x4 matWorld = float4x4(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);

    // matWorld엔 회전이 없다 (C++ 쪽에서 Scale * Translation만 곱함).
    // 중심 위치/스케일만 뽑아내고, 회전은 여기서 카메라 축으로 직접 만든다 (빌보드).
	float3 vCenter = float3(matWorld._41, matWorld._42, matWorld._43);
	float3 vRow0 = float3(matWorld._11, matWorld._12, matWorld._13);
	float fScale = length(vRow0);
	float3 vRight, vUp;
	vRight = normalize(float3(matWorld._11, matWorld._12, matWorld._13));
	vUp = normalize(float3(matWorld._21, matWorld._22, matWorld._23));

	float scaleX = length(float3(matWorld._11, matWorld._12, matWorld._13));
	float scaleY = length(float3(matWorld._21, matWorld._22, matWorld._23));

	float3 vWorldPos =
    vCenter +
    vRight * In.vPosition.x * scaleX +
    vUp * In.vPosition.y * scaleY;

	Out.vPosition = mul(float4(vWorldPos, 1.f), g_matViewProj);
	Out.vTexcoord = In.uvOffset + In.vTexcoord * In.uvSize;
	Out.vColor = In.vColor;
	Out.vEmissive = In.vInstEmissive;
	Out.vEndEmissive = In.vInstEndEmissive;
	Out.vScreenPos = Out.vPosition;
	Out.iBehaviorType = In.iBehaviorType;
	Out.vWorldPos = vWorldPos;
	Out.vTangent = vRight;
	Out.vNormal = normalize(cross(vRight, vUp));
	Out.life = In.life;
	Out.maxLife = In.maxLife;
    
	return Out;
}

struct PS_OUT
{
	float4 vDiffuse : SV_TARGET0;
};
PS_OUT PSMain(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float4 DiffuseTex	 = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
	
	float4 DistortionTex = g_DistortionTexture.Sample(LinearWrap, In.vTexcoord);

	float4 NoiseTex		 = g_NoiseTexture.Sample(LinearWrap, In.vTexcoord);

	float  Ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));
	 
	if (all(DiffuseTex.rgb <= 0.03f))
		discard;
	
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, Ratio);
    
	if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
	{
		clip(DiffuseTex.a - 0.02f);
		clip(In.vColor.a - 0.02f);

		float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
		screenUV.x = screenUV.x * 0.5f + 0.5f;
		screenUV.y = -screenUV.y * 0.5f + 0.5f;

		float4 vDistortionColor = g_DistortionTexture.Sample(LinearWrap, In.vTexcoord);
		float2 distortion = vDistortionColor.rg * 2.0f - 1.0f;

		float fEdgeMask = smoothstep(0.0f, 0.3f, DiffuseTex.a) *
                          (1.0f - smoothstep(0.3f, 0.9f, DiffuseTex.a));

		float distortionStrength = DistortionSTR * In.vColor.a * fEdgeMask;

		distortion *= distortionStrength;
		float4 distortedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion);
		float3 finalRGB = lerp(distortedBackground.rgb, DiffuseTex.rgb, DiffuseTex.a);
		finalRGB += lerpedEmissive.rgb * lerpedEmissive.a;

		Out.vDiffuse = float4(finalRGB, DiffuseTex.a);
		return Out;
	}
 
	float4 vFinalColor = DiffuseTex * In.vColor;;
	clip(vFinalColor.a - 0.02f);

	float3 WorldNormal = Compute_WorldNormal(
    g_NormalTexture,
    In.vTexcoord,
    In.vNormal,
    In.vTangent);
	float3 Albedo = pow(vFinalColor.rgb, 2.2f);

	float3 LightAccumulation = 0;

[unroll(MAX_LIGHT_COUNT)]
	for (int i = 0; i < LightCount; ++i)
	{
		float3 L, Radiance;

		if (!Compute_DynamicLight(AffectedLight[i], In.vWorldPos, L, Radiance))
			continue;

		float NDL = saturate(dot(WorldNormal, L));

		LightAccumulation += Albedo * Radiance * NDL;
	}

	float3 FinalColor = Albedo + LightAccumulation + lerpedEmissive.rgb * lerpedEmissive.a;

	Out.vDiffuse = float4(FinalColor, vFinalColor.a);
	return Out;
}
