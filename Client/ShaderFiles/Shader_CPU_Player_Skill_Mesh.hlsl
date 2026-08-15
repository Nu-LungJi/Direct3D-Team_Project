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

Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D SMROMap : register(t2);
Texture2D EmissiveMap : register(t3);
Texture2D NoiseMap : register(t5);
Texture2D DistortionMap : register(t6);
Texture2D g_BackgroundTex : register(t7);
Texture2D AnyTextureMap : register(t8);

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

PS_OUT PSAccio(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float2 diffuseUV =
        In.vTexcoord * float2(1.f, 1.f) + float2(0, g_fAccumulationTime * 0.2f);
	
	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));

	
	float4 texColor = AlbedoMap.Sample(LinearWrap, diffuseUV);

	if (all(texColor.rgb < 0.2f))
		discard;
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);

	float3 finalRGB = texColor.rgb * In.vColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;
	
	Out.vDiffuse = float4(finalRGB, texColor.a * In.vColor.a);
	return Out;
}

PS_OUT PSDepulso(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float dist = In.vTexcoord.y; // V: 구멍(0) ~ 바깥테두리(1)

	float bandWidth = 0.1f; // 링(띠)의 두께 — 값 조절 가능
	float edgeSoft = 0.05f;

    // 바깥 경계: 0에서 시작해서 1을 넘어(1+bandWidth)까지 이동 -> 끝에서 자연스럽게 사라짐
	float outerRadius = lerp(0.f, 1.f + bandWidth, ratio);
    // 안쪽 경계: 바깥보다 bandWidth만큼 뒤처져서 따라옴
	float innerRadius = outerRadius - bandWidth;

	float outerEdge = 1.f - smoothstep(outerRadius - edgeSoft, outerRadius, dist);
	float innerEdge = smoothstep(innerRadius - edgeSoft, innerRadius, dist);
	float alpha = outerEdge * innerEdge;

	float noise = AlbedoMap.Sample(LinearWrap, In.vTexcoord).r;
	alpha *= noise * In.vColor.a;
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float3 finalColor = In.vColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;		
	Out.vDiffuse = float4(finalColor, alpha);
	clip(alpha - 0.01f);

	return Out;
}

VS_OUT VSProtegoHitSurface(VS_IN In)
{
	VS_OUT Out = VSMain(In);

	// Keep the mesh-local direction. The particle instance now inherits the
	// effect world's hit rotation, so this local cap rotates with the sphere.
	Out.vBinormal = normalize(In.vNormal);
	return Out;
}

PS_OUT PSProtegoGlass(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float lifeRatio = saturate(In.life / max(In.maxLife, 0.0001f));
	float3 N = normalize(In.vNormal);
	float3 V = normalize(g_vCamPos - In.vWorldPos);
	float NdotV = saturate(abs(dot(N, V)));
	float fresnel = pow(1.0f - NdotV, 3.6f);
	float softRim = smoothstep(0.20f, 0.88f, fresnel);
	float thinRim = pow(1.0f - NdotV, 11.0f);
	// 월드 위치를 사용하면 보호막이 이동할 때 노이즈 표본이 바뀌어
	// 외곽선이 프레임마다 드르륵거린다. 구체의 로컬 방향에 패턴을 고정한다.
	float3 rippleCoord = N * 3.25f;
	float slowTime = g_fAccumulationTime * 0.72f;

	// Warp the sampling domain first so no wave keeps a straight, repeating path.
	float3 domainWarp;
	domainWarp.x = sin(rippleCoord.y * 1.13f + rippleCoord.z * 0.71f + slowTime * 1.07f);
	domainWarp.y = sin(rippleCoord.z * 0.93f - rippleCoord.x * 1.37f - slowTime * 0.83f);
	domainWarp.z = cos(rippleCoord.x * 0.79f + rippleCoord.y * 1.51f + slowTime * 0.61f);
	float3 warpedCoord = rippleCoord + domainWarp * 0.52f;

	float broadFlow = sin(warpedCoord.x * 1.47f + warpedCoord.y * 1.09f + slowTime * 1.31f);
	broadFlow += sin(warpedCoord.z * 1.83f - warpedCoord.y * 1.27f - slowTime * 0.97f) * 0.72f;
	float brokenFlow = sin(
		warpedCoord.x * 3.17f - warpedCoord.z * 2.41f +
		sin(warpedCoord.y * 2.23f - slowTime) * 1.35f + slowTime * 1.89f);
	float fineFlow = cos(
		(warpedCoord.x + warpedCoord.y - warpedCoord.z) * 4.61f -
		slowTime * 2.17f + broadFlow * 0.9f);

	float irregularFlow = broadFlow * 0.43f + brokenFlow * 0.37f + fineFlow * 0.20f;
	float surfaceRipple = saturate(0.5f + irregularFlow * 0.32f);
	float rippleHighlight = smoothstep(0.54f, 0.82f, surfaceRipple);
	rippleHighlight *= 0.62f + 0.38f * smoothstep(-0.25f, 0.75f, brokenFlow);
	float rippleValley = 1.0f - smoothstep(0.20f, 0.46f, surfaceRipple);

	// Asset-driven idle water. The turbulent water and caustic textures move in
	// opposite directions; each flow warps the other so it does not read as one
	// flat texture sliding over the sphere.
	float2 waterUV1 = In.vTexcoord * float2(1.72f, 1.34f) +
		float2(slowTime * 0.062f, -slowTime * 0.091f);
	float causticWarpA = dot(NoiseMap.Sample(LinearWrap, waterUV1).rgb,
		float3(0.299f, 0.587f, 0.114f));
	float2 waterUV2 = In.vTexcoord * float2(1.29f, 1.83f) +
		float2(-slowTime * 0.083f, slowTime * 0.054f);
	waterUV2 += float2(causticWarpA - 0.5f, 0.5f - causticWarpA) * 0.085f;
	float3 turbulentWater = AnyTextureMap.Sample(LinearWrap, waterUV2).rgb;
	float waterHeight = dot(turbulentWater, float3(0.299f, 0.587f, 0.114f));
	float2 reverseCausticUV = In.vTexcoord * 2.11f +
		float2(slowTime * 0.074f, slowTime * 0.103f) +
		(turbulentWater.rg - 0.5f) * 0.065f;
	float causticWarpB = dot(NoiseMap.Sample(LinearWrap, reverseCausticUV).rgb,
		float3(0.299f, 0.587f, 0.114f));
	float idleCaustic = smoothstep(0.38f, 0.78f,
		saturate(waterHeight * 0.56f + causticWarpA * 0.25f + causticWarpB * 0.29f));
	float idleBreath = 0.78f + 0.22f * sin(
		g_fAccumulationTime * 1.28f + broadFlow * 0.55f);

	// Moving broken rim: independent frequencies keep the outline from forming
	// one continuous, mechanically pulsing ring.
	float rimFlowA = sin(
		warpedCoord.x * 2.73f + warpedCoord.y * 3.91f -
		warpedCoord.z * 1.67f + slowTime * 2.37f + irregularFlow * 1.25f);
	float rimFlowB = cos(
		warpedCoord.x * 5.21f - warpedCoord.y * 2.19f +
		warpedCoord.z * 4.37f - slowTime * 1.63f + broadFlow * 0.74f);
	float rimFlowC = sin(
		(warpedCoord.x - warpedCoord.z) * 7.13f +
		warpedCoord.y * 1.31f + slowTime * 3.11f);
	float rimNoise = saturate(0.5f +
		(rimFlowA * 0.48f + rimFlowB * 0.34f + rimFlowC * 0.18f) * 0.5f);
	float brokenRim = smoothstep(0.40f, 0.72f, rimNoise);
	// 외곽선을 끊었다 붙이는 대신 밝기만 부드럽게 흐르게 한다.
	float rimVisibility = lerp(0.72f, 1.0f, brokenRim);
	float edgeSpark = smoothstep(0.78f, 0.94f, rimNoise);

	// Strong flowing sections grow inward and become visibly thicker. Weak
	// sections collapse back to the hairline silhouette or disappear.
	float thicknessFlow = smoothstep(0.40f, 0.78f,
		saturate(rimNoise * 0.68f + surfaceRipple * 0.32f));
	// 두께 변화 폭을 제한해 이동 중 실루엣이 계단처럼 튀지 않게 한다.
	float dynamicRimStart = lerp(0.58f, 0.42f, thicknessFlow);
	float variableThickRim = smoothstep(dynamicRimStart, 0.86f, fresnel);
	variableThickRim *= rimVisibility;
	float thickRimCrest = smoothstep(0.34f, 0.76f, variableThickRim);

	float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
	screenUV = screenUV * float2(0.5f, -0.5f) + 0.5f;
	float breath = 0.5f + 0.5f * sin(g_fAccumulationTime * 1.65f);
	float2 rippleDirection = normalize(N.xy + float2(0.001f, 0.001f));
	float2 refraction = N.xy * lerp(0.0030f, 0.0048f, breath) *
		(0.25f + fresnel * 0.75f);
	refraction += rippleDirection * (surfaceRipple - 0.5f) * 0.0075f;
	// Make the asset-driven flow deform the scene behind the resting shield.
	// This is what makes the motion read on transparent center pixels as well.
	float2 waterDistortion = (turbulentWater.rg - 0.5f) * 0.0330f;
	waterDistortion += float2(causticWarpA - causticWarpB,
		causticWarpB - causticWarpA) * 0.0110f;
	// A second, counter-flowing lobe breaks the single-direction UV slide and
	// produces clearly visible liquid wobble without making the glass opaque.
	float2 counterFlow = float2(causticWarpB - 0.5f,
		causticWarpA - 0.5f) * 0.0100f;
	waterDistortion += counterFlow * (0.55f + 0.45f * brokenFlow);
	refraction += waterDistortion * (0.58f + fresnel * 0.42f);
	float3 background = g_BackgroundTex.Sample(LinearClamp, screenUV + refraction).rgb;

	float3 glassTint = float3(0.30f, 0.08f, 0.62f);
	float3 darkPurpleRim = float3(0.055f, 0.012f, 0.085f);
	float3 blackVioletRim = float3(0.012f, 0.006f, 0.028f);
	float3 violetGlint = float3(0.66f, 0.12f, 0.92f);
	float3 whiteEdge = float3(0.42f, 0.20f, 0.52f);
	float verticalBlend = saturate(0.5f + 0.5f * N.y);
	float3 rimTint = lerp(blackVioletRim, darkPurpleRim, verticalBlend);

	float3 finalColor = background;
	float centerGlass = 1.0f - softRim * 0.72f;
	float3 centerTint = lerp(float3(0.16f, 0.08f, 0.48f),
		float3(0.42f, 0.10f, 0.68f), verticalBlend);
	finalColor = lerp(finalColor, finalColor * 0.86f + centerTint * 0.14f,
		0.62f * centerGlass);
	finalColor += lerp(float3(0.30f, 0.18f, 1.0f),
		float3(0.88f, 0.08f, 1.0f), surfaceRipple) *
		rippleHighlight * centerGlass * 0.24f;
	finalColor += lerp(float3(0.20f, 0.10f, 0.72f),
		float3(0.62f, 0.20f, 1.0f), surfaceRipple) *
		idleCaustic * idleBreath * centerGlass * 0.46f;
	float waterShadow = 1.0f - smoothstep(0.18f, 0.48f, waterHeight);
	finalColor *= 1.0f - waterShadow * centerGlass * 0.11f;
	finalColor *= 1.0f - rippleValley * centerGlass * 0.055f;
	finalColor += glassTint * fresnel * 0.12f;
	finalColor += rimTint * softRim * (0.10f + rimVisibility * 1.38f);
	finalColor += lerp(rimTint, violetGlint, thicknessFlow) *
		variableThickRim * 1.38f;
	finalColor += whiteEdge * thinRim * (0.025f + rimVisibility * 0.82f);
	finalColor += violetGlint * edgeSpark *
		(softRim * 0.24f + thickRimCrest * 0.62f);

	float fadeIn = smoothstep(0.0f, 0.055f, lifeRatio);
	float fadeOut = 1.0f - smoothstep(0.82f, 1.0f, lifeRatio);
	float alpha = saturate(0.115f + centerGlass * 0.075f +
		rippleHighlight * centerGlass * 0.11f +
		idleCaustic * idleBreath * centerGlass * 0.075f +
		rippleValley * centerGlass * 0.035f +
		fresnel * 0.24f +
		softRim * (0.008f + rimVisibility * 0.25f) +
		variableThickRim * 0.52f +
		thinRim * (0.01f + rimVisibility * 0.72f));
	alpha *= fadeIn * fadeOut * In.vColor.a;

	Out.vDiffuse = float4(finalColor, alpha);
	return Out;
}

PS_OUT PSProtegoHitSurface(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float3 localDirection = normalize(In.vBinormal);

	// Only retain a spherical cap around local +Z. Since these pixels are
	// rendered by the shield sphere itself, the decal follows the surface
	// exactly instead of floating on a tangent quad.
	const float capCos = 0.72f;
	float capMask = smoothstep(capCos, capCos + 0.055f, localDirection.z);
	float capRadius = sqrt(1.0f - capCos * capCos);
	float2 hitUV = localDirection.xy / capRadius * 0.5f + 0.5f;

	float2 interferenceUV = hitUV * 1.31f +
		float2(-g_fAccumulationTime * 0.13f, g_fAccumulationTime * 0.21f);
	float3 interference = NoiseMap.Sample(LinearWrap, interferenceUV).rgb;
	float interferenceWhite = saturate(dot(interference, float3(0.299f, 0.587f, 0.114f)));
	float2 reverseFlowUV = hitUV * 1.73f +
		float2(g_fAccumulationTime * 0.17f, -g_fAccumulationTime * 0.09f);
	float reverseNoise = dot(NoiseMap.Sample(LinearWrap, reverseFlowUV).rgb,
		float3(0.299f, 0.587f, 0.114f));

	float2 centeredHitUV = hitUV - 0.5f;
	float twist = (interferenceWhite - reverseNoise) * 0.34f +
		sin(g_fAccumulationTime * 4.7f + reverseNoise * 5.0f) * 0.055f;
	float sinTwist = sin(twist);
	float cosTwist = cos(twist);
	float2 twistedUV = float2(
		centeredHitUV.x * cosTwist - centeredHitUV.y * sinTwist,
		centeredHitUV.x * sinTwist + centeredHitUV.y * cosTwist);
	float breathing = 1.0f + sin(g_fAccumulationTime * 5.3f + interferenceWhite * 3.0f) * 0.045f;
	// Contracting sample coordinates make the white body spread across the
	// spherical surface. Two independent noise flows curl its edge like smoke.
	float smokeSpread = lerp(1.08f, 0.60f, smoothstep(0.0f, 0.78f, ratio));
	float2 radialDirection = centeredHitUV / max(length(centeredHitUV), 0.025f);
	float curl = (interferenceWhite - reverseNoise) * (0.045f + ratio * 0.075f);
	float2 blastUV = twistedUV * breathing * smokeSpread + 0.5f;
	blastUV += float2(interferenceWhite - 0.5f, reverseNoise - 0.5f) *
		(0.055f + ratio * 0.085f);
	blastUV += float2(-radialDirection.y, radialDirection.x) * curl;
	float3 blast = AnyTextureMap.Sample(LinearWrap, blastUV).rgb;
	float blastWhite = saturate(dot(blast, float3(0.299f, 0.587f, 0.114f)));
	float smokyBody = saturate(blastWhite * (0.74f + interferenceWhite * 0.52f));
	float wispyBreakup = smoothstep(0.24f, 0.76f,
		saturate(interferenceWhite * 0.62f + reverseNoise * 0.38f));
	float overlap = pow(saturate(smokyBody * interferenceWhite), 1.15f);

	float radial = length(hitUV * 2.0f - 1.0f);
	float brokenEdge = 1.0f - smoothstep(0.035f, 0.13f,
		abs(radial - lerp(0.15f, 0.78f, ratio) - (interferenceWhite - 0.5f) * 0.10f));
	float fadeIn = smoothstep(0.0f, 0.045f, ratio);
	float fadeOut = 1.0f - smoothstep(0.91f, 1.0f, ratio);
	float dissolveProgress = smoothstep(0.68f, 0.99f, ratio);
	float dissolveNoise = saturate(interferenceWhite * 0.60f + reverseNoise * 0.40f);
	float dissolveMask = smoothstep(dissolveProgress - 0.12f,
		dissolveProgress + 0.13f, dissolveNoise);
	float dissolveEdge = 1.0f - smoothstep(0.025f, 0.105f,
		abs(dissolveNoise - dissolveProgress));
	float lingeringWisps = smokyBody * wispyBreakup *
		(1.0f - smoothstep(0.86f, 1.0f, ratio));
	float alpha = saturate(smokyBody * (0.78f + wispyBreakup * 0.52f) +
		lingeringWisps * 0.42f +
		overlap * 0.52f + brokenEdge * 0.55f);
	alpha *= capMask * fadeIn * fadeOut * dissolveMask * In.vColor.a;
	clip(alpha - 0.018f);

	float3 whiteBurst = blast * (0.92f + smokyBody * 2.05f) *
		lerp(1.0f, 0.58f, ratio);
	whiteBurst += float3(0.74f, 0.66f, 0.92f) * lingeringWisps * 0.48f;
	float3 violetOverlap = lerp(float3(0.36f, 0.06f, 0.82f),
		float3(0.98f, 0.22f, 1.0f), interferenceWhite) * overlap * 4.8f;
	float3 edgeColor = float3(0.66f, 0.24f, 1.0f) * brokenEdge * 2.2f;
	float3 dissolveColor = float3(0.78f, 0.38f, 1.0f) * dissolveEdge *
		capMask * fadeOut * 1.65f;
	Out.vDiffuse = float4(whiteBurst + violetOverlap + edgeColor + dissolveColor, alpha);
	return Out;
}

PS_OUT PSProtegoHitPulse(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float2 flowUV1 = In.vTexcoord * 2.15f +
		float2(g_fAccumulationTime * 0.19f, -g_fAccumulationTime * 0.11f);
	float2 flowUV2 = In.vTexcoord * 1.43f +
		float2(-g_fAccumulationTime * 0.12f, g_fAccumulationTime * 0.23f);
	float noiseA = dot(NoiseMap.Sample(LinearWrap, flowUV1).rgb,
		float3(0.299f, 0.587f, 0.114f));
	float noiseB = dot(AnyTextureMap.Sample(LinearWrap, flowUV2).rgb,
		float3(0.299f, 0.587f, 0.114f));
	float irregular = saturate(noiseA * 0.58f + noiseB * 0.42f);
	float3 localDirection = normalize(In.vBinormal);
	float angularDistance = acos(clamp(localDirection.z, -1.0f, 1.0f)) / PI;

	float pulseEnvelope = sin(saturate(ratio) * PI);
	float movingVein = smoothstep(0.42f, 0.76f,
		saturate(irregular + sin(In.vWorldPos.y * 2.7f - g_fAccumulationTime * 5.2f) * 0.16f));
	float wholeShieldShimmer = movingVein * pulseEnvelope;

	// A broad, noise-warped front reads as a wave without becoming a perfect ring.
	float waveNoise = (noiseA - 0.5f) * 0.060f + (noiseB - 0.5f) * 0.038f;
	float waveCenter = smoothstep(0.0f, 1.0f, ratio) * 0.90f;
	float waveDistance = abs(angularDistance - waveCenter - waveNoise);
	float broadWave = 1.0f - smoothstep(0.035f, 0.125f, waveDistance);
	float brokenWave = smoothstep(0.25f, 0.76f,
		saturate(irregular + sin(localDirection.x * 11.0f + localDirection.y * 8.0f - g_fAccumulationTime * 4.3f) * 0.14f));
	float travellingWave = broadWave * lerp(0.38f, 1.0f, brokenWave) * pulseEnvelope;

	float echoCenter = waveCenter - 0.115f;
	float echoDistance = abs(angularDistance - echoCenter - waveNoise * 0.62f);
	float softEcho = (1.0f - smoothstep(0.028f, 0.105f, echoDistance)) *
		(0.35f + irregular * 0.30f) * pulseEnvelope;
	float hitBloom = (1.0f - smoothstep(0.0f, 0.16f, angularDistance)) *
		(1.0f - smoothstep(0.18f, 0.62f, ratio));
	// After the wave passes, dissolve the temporary HIT sphere from the impact
	// point toward the opposite pole. The base Protego sphere is a separate
	// particle and remains untouched underneath this layer.
	float dissolveRadius = smoothstep(0.44f, 1.0f, ratio) * 1.12f;
	float dissolveWarp = (noiseA - 0.5f) * 0.105f +
		(noiseB - 0.5f) * 0.072f;
	float warpedDissolveDistance = angularDistance + dissolveWarp;
	float surfaceDissolve = smoothstep(dissolveRadius - 0.075f,
		dissolveRadius + 0.105f, warpedDissolveDistance);
	float surfaceDissolveEdge = 1.0f - smoothstep(0.018f, 0.085f,
		abs(warpedDissolveDistance - dissolveRadius));
	surfaceDissolveEdge *= smoothstep(0.42f, 0.58f, ratio);

	float3 N = normalize(In.vNormal);
	float3 V = normalize(g_vCamPos - In.vWorldPos);
	float fresnel = pow(1.0f - saturate(abs(dot(N, V))), 3.2f);
	float alpha = saturate(
		travellingWave * 0.52f + softEcho * 0.25f + hitBloom * 0.32f +
		wholeShieldShimmer * 0.25f + fresnel * pulseEnvelope * 0.20f);
	alpha *= surfaceDissolve * In.vColor.a;
	clip(alpha - 0.012f);

	float3 deepViolet = float3(0.20f, 0.025f, 0.68f);
	float3 brightViolet = float3(0.82f, 0.16f, 1.0f);
	float3 pulseColor = lerp(deepViolet, brightViolet, irregular);
	pulseColor *= wholeShieldShimmer * 1.75f + travellingWave * 2.65f + softEcho * 1.10f + hitBloom * 1.80f;
	pulseColor += float3(0.62f, 0.28f, 1.0f) * fresnel * pulseEnvelope * 1.05f;
	pulseColor += float3(0.88f, 0.30f, 1.0f) * surfaceDissolveEdge *
		(0.75f + irregular * 0.55f) * 1.35f;
	Out.vDiffuse = float4(pulseColor, alpha);
	return Out;
}
