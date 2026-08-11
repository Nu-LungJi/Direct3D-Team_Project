#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

#define MAX_LIGHT_COUNT     8
#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2
cbuffer CB_TIMEACCUMULATION : register(b11)
{
	float g_fAccumulationTime;
	float3 _pad;
};

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

	float4 vWorld0 : INSTANCE_WORLD0;
	float4 vWorld1 : INSTANCE_WORLD1;
	float4 vWorld2 : INSTANCE_WORLD2;
	float4 vWorld3 : INSTANCE_WORLD3;

	float4 vColor : INSTANCE_COLOR0;

	float4 vInstOriginalEmissive : INSTANCE_EMISSIVE0;
	float4 vInstEmissive : INSTANCE_EMISSIVE1;
	float4 vInstEndEmissive : INSTANCE_EMISSIVE2;

	float2 uvOffset : INSTANCE_UVOFFSET;
	float2 uvSize : INSTANCE_UVSIZE;

	float life : INSTANCE_LIFE;
	float maxLife : INSTANCE_MAXLIFE;

	uint iBehaviorType : INSTANCE_BEHAVIORTYPE;
};

struct VS_OUT
{
	float4 vPosition : SV_POSITION;
	float2 vTexcoord : TEXCOORD0;
	float3 vWorldPos : TEXCOORD1;

	float life : TEXCOORD2;
	float maxLife : TEXCOORD3;

	nointerpolation uint iBehaviorType : TEXCOORD4;

	float4 vScreenPos : TEXCOORD5;
	float4 vColor : COLOR0;

	float3 vNormal : NORMAL0;
	float3 vTangent : TANGENT0;
	float3 vBinormal : BINORMAL0;

	float4 vEmissive : EMISSIVE0;
	float4 vEndEmissive : EMISSIVE1;
};

VS_OUT VSMain(VS_IN In)
{
	VS_OUT Out = (VS_OUT) 0;

	float4x4 matWorld = float4x4(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);
	float4 worldPosition = mul(float4(In.vPosition, 1.f), matWorld);

	Out.vPosition = mul(worldPosition, g_matViewProj);
	Out.vWorldPos = worldPosition.xyz;
	Out.vScreenPos = Out.vPosition;

	Out.vTexcoord = In.uvOffset + In.vTexcoord * In.uvSize;

	float3x3 world3x3 = (float3x3) matWorld;

	Out.vNormal = normalize(mul(In.vNormal, world3x3));
	Out.vTangent = normalize(mul(In.vTangent, world3x3));
	Out.vBinormal = normalize(mul(In.vBinormal, world3x3));

	Out.vColor = In.vColor;
	Out.vEmissive = In.vInstEmissive;
	Out.vEndEmissive = In.vInstEndEmissive;

	Out.life = In.life;
	Out.maxLife = In.maxLife;
	Out.iBehaviorType = In.iBehaviorType;

	return Out;
}
// ---- 텍스처 기반 버텍스 디스토션(WPO 스타일) 파라미터 ----
// 필요하면 상수버퍼로 빼서 인스턴스/머티리얼별로 노출하세요.
static const float DistortTilingU = 1.0f;
static const float DistortTilingV = 1.35f;
static const float DistortSpeedU = 1.12f;
static const float DistortSpeedV = 1.4875f;
static const float DistortStrength = 0.91f;
static const float DistortMipLevel = 0.0f; // VS는 자동 밉 계산이 없어서 명시 지정 필수


VS_OUT VSSwirlDistortTex(VS_IN In)
{
	VS_OUT Out = (VS_OUT)0;
	float4x4 matWorld = float4x4(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);
	float3 localPosition = In.vPosition;

	static const float TendrilRadius = 40.f;

	float localRadius = length(localPosition);
	float bodyPosition = saturate(localRadius / TendrilRadius);
	float rootMask = smoothstep(0.15f, 0.35f, bodyPosition);
	float bendMask = rootMask * bodyPosition * bodyPosition;
	float instancePhase = dot(matWorld[3].xyz, float3(0.173f, 0.317f,
	0.271f));
	float2 noiseUV = localPosition.xz * 0.025f * float2(DistortTilingU, DistortTilingV);
	noiseUV += float2(g_fAccumulationTime * DistortSpeedU, -g_fAccumulationTime * DistortSpeedV);
	noiseUV += instancePhase;

	float2 noise = AnyTextureMap.SampleLevel(LinearWrap, noiseUV, DistortMipLevel).rg * 2.f - 1.f;
	float3 radialDirection = localPosition / max(localRadius, 0.0001f);
	float3 sideAxis = cross(float3(0.f, 1.f, 0.f), radialDirection);
	if (dot(sideAxis, sideAxis) < 0.0001f)
		sideAxis = cross(float3(1.f, 0.f, 0.f), radialDirection);
	sideAxis = normalize(sideAxis);
	float3 bendAxis = normalize(cross(radialDirection, sideAxis));
	float wave1 = sin(bodyPosition * 6.f - g_fAccumulationTime * 2.2f + instancePhase + noise.x);
	float wave2 = cos(bodyPosition * 8.f + g_fAccumulationTime * 1.6f + instancePhase * 1.37f + noise.y);
	float bendStrength = DistortStrength * 180.f;

	localPosition += sideAxis * wave1 * bendStrength * bendMask;
	localPosition += bendAxis * wave2 * bendStrength * 0.45f * bendMask;

	float4 worldPosition = mul(float4(localPosition, 1.f), matWorld);
	Out.vPosition = mul(worldPosition, g_matViewProj);
	Out.vWorldPos = worldPosition.xyz;
	Out.vScreenPos = Out.vPosition;
	Out.vTexcoord = In.uvOffset + In.vTexcoord * In.uvSize;

	float3x3 world3x3 = (float3x3)matWorld;
	Out.vNormal = normalize(mul(In.vNormal, world3x3));
	Out.vTangent = normalize(mul(In.vTangent, world3x3));
	Out.vBinormal = normalize(mul(In.vBinormal, world3x3));
	Out.vColor = In.vColor;
	Out.vEmissive = In.vInstEmissive;
	Out.vEndEmissive = In.vInstEndEmissive;
	Out.life = In.life;
	Out.maxLife = In.maxLife;
	Out.iBehaviorType = In.iBehaviorType;

	return Out;
}
struct PS_OUT
{
	float4 vDiffuse : SV_TARGET0;
};

PS_OUT PSMain(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float4 AlbedoTex = AlbedoMap.Sample(LinearWrap, In.vTexcoord);
	AlbedoTex *= In.vColor;

	clip(AlbedoTex.a - 0.05f);

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float3 Albedo = pow(max(AlbedoTex.rgb, 0.f), 2.2f);

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
		float3 L;
		float3 Radiance;

		if (!Compute_DynamicLight(AffectedLight[i], In.vWorldPos, L, Radiance))
			continue;

		float RawNDL = dot(WorldNormal, L);

		if (RawNDL <= 0.f)
			continue;

		float NDL = saturate(RawNDL);
		float3 H = normalize(V + L);

		float D = DistributionGGX(WorldNormal, H, fRoughness);
		float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);
		float V_Spec = VisibilitySmithJointGGX(NDV, NDL, fRoughness);

		float3 Specular = D * F * V_Spec * SpecularIntensity;
		float3 kS = F;
		float3 kD = (1.f - kS) * (1.f - fMetallic);
		float3 Diffuse = kD * Albedo / PI;

		LightAccumulation += (Diffuse + Specular) * Radiance * NDL;
	}

	float3 texEmissive = EmissiveMap.Sample(LinearWrap, In.vTexcoord).rgb;
	texEmissive = pow(max(texEmissive, 0.f), 2.2f);

	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float3 instanceEmissive = lerpedEmissive.rgb * lerpedEmissive.a;

	float3 constantAmbient = Albedo * 0.05f * fAmbient;
	float3 finalColor = constantAmbient + LightAccumulation + texEmissive + instanceEmissive;

	Out.vDiffuse = float4(finalColor, AlbedoTex.a);

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
PS_OUT PSPlayerDashWindSpiral(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float2 uv = In.vTexcoord;

    /*
     * 메쉬 UV의 U축을 따라 바람이 흐르게 한다.
     * 반대로 흐르면 부호를 바꾼다.
     */
	float2 diffuseUV =
        uv * float2(4.f, 4.f) +
        float2(-g_fAccumulationTime * 1.2f, 0.f);

	float2 normalUV =
        uv * float2(2.f, 2.f) +
        float2(-g_fAccumulationTime * 0.55f, 0.f);

	float2 noiseUV =
        uv * float2(3.f, 2.f) +
        float2(-g_fAccumulationTime * 0.8f, 0.f);

	float2 distortion =
        NormalMap.Sample(LinearWrap, normalUV).rg * 2.f - 1.f;

	diffuseUV += distortion * 0.03f;
	noiseUV += distortion * 0.02f;

	float4 diffuse =
        AlbedoMap.Sample(LinearWrap, diffuseUV);

	float noise =
        NoiseMap.Sample(LinearWrap, noiseUV).r;

    // 텍스처가 흑백일 경우 밝기를 형태 마스크로 사용
	float shapeMask =
        max(diffuse.r, max(diffuse.g, diffuse.b));

    // 흐르면서 윤곽이 생성되고 사라지는 부분
	float erosion =
        smoothstep(0.25f, 0.7f, noise);

    // 메쉬 띠의 양쪽 가장자리 페이드
    // Fat 메쉬의 V축이 띠의 너비 방향이라는 전제
	float edgeFade =
        1.f - abs(uv.y * 2.f - 1.f);

	edgeFade = smoothstep(0.f, 0.35f, edgeFade);
	
	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float fadeIn =
        smoothstep(0.f, 0.08f, ratio);

	float fadeOut =
        1.f - smoothstep(0.65f, 1.f, ratio);

	float alpha =
        shapeMask *
        lerp(1.f, erosion, 0.65f) *
        edgeFade *
        fadeIn *
        fadeOut *
        In.vColor.a;

	float3 windColor =
        float3(0.72f, 0.88f, 1.f);

	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float3 instanceEmissive = lerpedEmissive.rgb * lerpedEmissive.a;
    // 밝은 중심부
	float emissive =
        pow(saturate(shapeMask), 1.5f) * 0.35f;


	float3 finalColor = In.vColor.rgb  + instanceEmissive;

	Out.vDiffuse = float4(finalColor, alpha);

	clip(alpha - 0.002f);

	return Out;
}
PS_OUT PSAccio(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float2 diffuseUV =
        In.vTexcoord * float2(1.f, 1.f) + float2(0, g_fAccumulationTime * 0.2f);
	
	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));

	
	float4 texColor = AlbedoMap.Sample(LinearWrap, diffuseUV);

	if (all(texColor.rgb < 0.2f))
		discard;
	texColor.rgb = pow(texColor.rgb, 2.5f);
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);

	float3 finalRGB = texColor.rgb * In.vColor.rgb * lerpedEmissive.rgb * lerpedEmissive.a;
	
	Out.vDiffuse = float4(finalRGB,   In.vColor.a);
	return Out;
}
PS_OUT PSMarble(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

    // 크랙 마스크 샘플링 (마블 텍스처: 검은 배경 + 흰 균열선)
	
	float2 uv = In.vTexcoord;
	uv.y += g_fAccumulationTime * 0.1f;	
	float crackMask = AnyTextureMap.Sample(LinearWrap, uv).g;
	crackMask = saturate(pow(crackMask, 2.0f));
    // 베이스는 인스턴스 컬러 그대로
	float3 baseColor = In.vColor.rgb;

    // 마스크 있는 부분만 In.vEmissive 색으로 발광 (알파를 강도로 사용)
	float3 emissive = crackMask * In.vEmissive.rgb * In.vEmissive.a;

	float3 finalColor = baseColor + emissive;

	Out.vDiffuse = float4(finalColor, In.vColor.a);
	return Out;
}

PS_OUT PSBreathe(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float2 flameUV = In.vTexcoord * float2(2.f, 5.f);
	float2 noiseUV = In.vTexcoord * float2(3.f, 4.f);

	flameUV.y += g_fAccumulationTime * 0.8f;
	noiseUV.y -= g_fAccumulationTime * 0.35f;
	noiseUV.x += g_fAccumulationTime * 0.1f;

	float noise = NoiseMap.Sample(LinearWrap, noiseUV).r;
	float2 distortion = (noise * 2.f - 1.f) * float2(0.04f, 0.02f);
	float flame = AlbedoMap.Sample(LinearWrap, flameUV + distortion).r;
	float pattern = smoothstep(0.08f, 0.85f, saturate(flame * 0.75f + noise * 0.25f));

	float lifeRatio = saturate(In.life / max(In.maxLife, 0.0001f));
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);

	float3 baseColor = In.vColor.rgb;	
	float3 emissive = pattern * lerpedEmissive.rgb * lerpedEmissive.a;
	float3 finalColor = baseColor + emissive;
	float alpha = lerp(0.4f, 0.9f, pattern) * In.vColor.a;

	Out.vDiffuse = float4(finalColor, alpha);
	return Out;
}
PS_OUT PSDrgonProj(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float2 uv = In.vTexcoord;
	uv.x += g_fAccumulationTime * 1.13f;
	uv.y -= g_fAccumulationTime * 1.13f;

	float4 surfaceTex = AlbedoMap.Sample(LinearWrap, uv);
	float3 emissiveTex = EmissiveMap.Sample(LinearWrap, uv).rgb;
	float surfaceValue = max(surfaceTex.r, max(surfaceTex.g, surfaceTex.b));
	float emissiveValue = max(emissiveTex.r, max(emissiveTex.g, emissiveTex.b));
	float veinMask = pow(saturate(surfaceValue * emissiveValue), 2.f);

	float2 normalUV1 = In.vTexcoord + float2(g_fAccumulationTime * 0.035f, -g_fAccumulationTime * 0.055f);
	float2 normalUV2 = In.vTexcoord * 1.7f + float2(-g_fAccumulationTime * 0.045f, g_fAccumulationTime * 0.025f);
	float2 normal1 = NormalMap.Sample(LinearWrap, normalUV1).rg * 2.f - 1.f;
	float2 normal2 = NormalMap.Sample(LinearWrap, normalUV2).rg * 2.f - 1.f;
	float2 distortion = (normal1 + normal2) * 0.007f;

	float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
	screenUV = screenUV * float2(0.5f, -0.5f) + 0.5f;

	float3 N = normalize(In.vNormal);
	float3 V = normalize(g_vCamPos - In.vWorldPos);
	float fresnel = pow(1.f - saturate(dot(N, V)), 3.f);

	distortion *= 0.5f + fresnel;
	float3 background = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion).rgb;

	float lifeRatio = saturate(1.f - In.life / max(In.maxLife, 0.0001f));
	float4 emissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);
	float shellMask = smoothstep(0.05f, 0.65f, surfaceValue);
	float3 liquidTint = In.vColor.rgb * shellMask * In.vColor.a * 0.15f;
	float3 rimColor = emissive.rgb * emissive.a * fresnel * 0.2f;
	float3 veinColor = emissive.rgb * emissive.a * veinMask;
	float3 finalColor = background + liquidTint + rimColor + veinColor;

	Out.vDiffuse = float4(finalColor, 1.f);
	return Out;
}
PS_OUT PSWater(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float2 surfaceUV1 = In.vTexcoord + float2(-g_fAccumulationTime * 0.02f, -g_fAccumulationTime * 0.12f);
	float2 surfaceUV2 = In.vTexcoord * 1.7f + float2(g_fAccumulationTime * 0.08f, g_fAccumulationTime * 0.08f);
	float surface1 = AlbedoMap.Sample(LinearWrap, surfaceUV1).r;
	float surface2 = AlbedoMap.Sample(LinearWrap, surfaceUV2).r;
	float surfaceFlow = saturate(surface1 * 0.65f + surface2 * 0.35f);

	float2 normalUV1 = In.vTexcoord + float2(-g_fAccumulationTime * 0.1f, -g_fAccumulationTime * 1.2f);
	float2 normalUV2 = In.vTexcoord * 1.6f + float2(g_fAccumulationTime * 0.07f, g_fAccumulationTime * 0.6f);
	float2 normal1 = NormalMap.Sample(LinearWrap, normalUV1).rg * 2.f - 1.f;
	float2 normal2 = NormalMap.Sample(LinearWrap, normalUV2).rg * 2.f - 1.f;
	float2 waterNormal = normalize(normal1 + normal2 + 0.0001f);

	float3 N = normalize(In.vNormal);
	float3 V = normalize(g_vCamPos - In.vWorldPos);
	float NdotV = saturate(dot(N, V));
	float fresnel = pow(1.f - NdotV, 4.f);
	float thinRim = pow(1.f - NdotV, 10.f);

	float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;    
	screenUV = screenUV * float2(0.5f, -0.5f) + 0.5f;
	float2 distortion = waterNormal * 0.012f * lerp(0.4f, 1.f, fresnel);
	float3 background = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion).rgb;

	float lifeRatio = saturate(In.life / max(In.maxLife, 0.0001f));
	float4 emissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);
	float gradientMask = AnyTextureMap.Sample(LinearWrap, In.vTexcoord).r;
	float interiorMask = saturate(surfaceFlow * 0.6f + gradientMask * 0.4f);
	float absorption = interiorMask * In.vColor.a * 0.08f;
	float3 waterTint = lerp(float3(1.f, 1.f, 1.f), In.vColor.rgb, 0.2f);
	float3 transparentWater = background * lerp(float3(1.f, 1.f, 1.f), waterTint, absorption);
	float emissiveIntensity = min(emissive.a, 1.5f);
	float3 surfaceColor = In.vColor.rgb * surfaceFlow * In.vColor.a * 0.02f;
	float3 rimColor = emissive.rgb * emissiveIntensity * fresnel * 0.45f;
	float3 whiteRim = lerp(emissive.rgb, float3(1.f, 0.9f, 0.85f), 0.7f) * thinRim * 1.2f;
	float3 finalColor = transparentWater + surfaceColor + rimColor + whiteRim;

	Out.vDiffuse = float4(finalColor, In.vColor.a);
	
	return Out;
}
PS_OUT PSOuterSphere(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;


	float lifeRatio = saturate(In.life / max(In.maxLife, 0.0001f));
	float4 emissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);

	float2 diffuseUV1 = In.vTexcoord + float2(-g_fAccumulationTime * 0.02f, -g_fAccumulationTime * 0.12f);
	float2 diffuseUV2 = In.vTexcoord * 1.7f + float2(g_fAccumulationTime * 0.06f, g_fAccumulationTime * 0.08f);
	float3 diffuseTex1 = AlbedoMap.Sample(LinearWrap, diffuseUV1).rgb;
	float3 diffuseTex2 = AlbedoMap.Sample(LinearWrap, diffuseUV2).rgb;
	float3 diffuseTex = diffuseTex1 * 0.65f + diffuseTex2 * 0.35f;
	float diffuseMask = dot(diffuseTex, float3(0.299f, 0.587f, 0.114f));

	float3 geometryNormal = normalize(In.vNormal);
	float3 viewDirection = normalize(g_vCamPos - In.vWorldPos);
	float NdotV = saturate(dot(geometryNormal, viewDirection));
	float rim = 1.f - NdotV;

	float redHaloMask = smoothstep(0.15f, 0.82f, rim);
	float outerLineMask = smoothstep(0.6f, 0.96f, rim);
	float whiteCoreMask = smoothstep(0.96f, 0.99f, rim);

	
	
	float textureMask = smoothstep(0.15f, 0.8f, diffuseMask);
	float3 bodyColor = In.vColor.rgb * lerp(0.15f, 0.8f, textureMask);
	float bodyAlpha = In.vColor.a * lerp(0.35f, 1.f, textureMask);
	
	
	float3 redHaloColor = In.vColor.rgb * redHaloMask * 0.8f;
	float3 outerLineColor = emissive.rgb * emissive.a * outerLineMask * 1.2f;
	float3 whiteCoreColor = lerp(emissive.rgb, float3(1.f, 0.95f, 0.9f), 0.8f) * emissive.a * whiteCoreMask * 2.f;
	float3 finalColor = bodyColor + redHaloColor + outerLineColor + whiteCoreColor;

	float redHaloAlpha = redHaloMask * 0.18f;
	float outerLineAlpha = outerLineMask * 0.65f;
	float whiteCoreAlpha = whiteCoreMask * 0.35f;
	float finalAlpha = saturate(bodyAlpha + redHaloAlpha + outerLineAlpha + whiteCoreAlpha);

	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}
