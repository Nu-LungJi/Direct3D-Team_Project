#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"

#define BEHAVIOR_NONE       0
#define BEHAVIOR_DISTORTION (1u << 1)
#define BEHAVIOR_BILLBOARD  (1u << 2)

cbuffer CB_TIMEACCUMULATION : register(b11)
{
	float g_fAccumulationTime;
	float3 _pad;
};
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
	float4 vInstOriginalEmissive : INSTANCE_EMISSIVE0;
	float4 vInstEmissive : INSTANCE_EMISSIVE1;
	float4 vInstEndEmissive : INSTANCE_EMISSIVE2;
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

Texture2D g_DiffuseTexture : register(t1);
Texture2D g_NormalTexture : register(t2);
Texture2D g_DistortionTexture : register(t3);
Texture2D g_NoiseTexture : register(t4);
Texture2D g_AnyTexture : register(t5);
Texture2D g_BackgroundTex : register(t7);

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;


};
PS_OUT PSMain(VS_OUT In)
{
 
    PS_OUT Out = (PS_OUT) 0;

    float4 texColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
    float4 vDistortionColor = g_DistortionTexture.Sample(LinearWrap, In.vTexcoord);
	float4 noise = g_NoiseTexture.Sample(LinearWrap, In.vTexcoord);
    

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));

	//if (noise.r < ratio) 
	//	discard;
    if (all(texColor.rgb <= 0.03f))
        discard;
    float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
    
    if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
    {
        clip(texColor.a - 0.02f);
        clip(In.vColor.a - 0.02f);

        float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
        screenUV.x = screenUV.x * 0.5f + 0.5f;
        screenUV.y = -screenUV.y * 0.5f + 0.5f;

        float4 vDistortionColor = g_DistortionTexture.Sample(LinearWrap, In.vTexcoord);
        float2 distortion = vDistortionColor.rg * 2.0f - 1.0f;

        float fEdgeMask = smoothstep(0.0f, 0.3f, texColor.a) *
                          (1.0f - smoothstep(0.3f, 0.9f, texColor.a));

        float distortionStrength = 0.05f * In.vColor.a * fEdgeMask;

        distortion *= distortionStrength;
        float4 distortedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion);
        float3 finalRGB = lerp(distortedBackground.rgb, texColor.rgb, texColor.a);
		
        finalRGB += lerpedEmissive.rgb * lerpedEmissive.a;

        Out.vDiffuse = float4(finalRGB, 1.0f);
        return Out;
    }
 
    float4 vFinalColor = texColor * In.vColor;
    clip(vFinalColor.a - 0.02f);
    float3 Albedo = pow(vFinalColor.rgb, 2.2f);

    float3 LightAccumulation = 0;


    float3 FinalColor =Albedo +LightAccumulation +lerpedEmissive.rgb * lerpedEmissive.a;

    Out.vDiffuse = float4(FinalColor, vFinalColor.a);
    return Out;
}


PS_OUT RemoveBlack(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));

	float4 texColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);

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
	
	return Out;
}

PS_OUT PSProtegoImpact(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float2 centeredUV = In.vTexcoord * 2.0f - 1.0f;
	float radius = length(centeredUV);
	float angle = atan2(centeredUV.y, centeredUV.x);

	float2 membraneUV = In.vTexcoord;
	membraneUV += float2(
		sin(angle * 5.0f + g_fAccumulationTime * 11.0f),
		cos(angle * 7.0f - g_fAccumulationTime * 9.0f)) * 0.018f;
	float3 membrane = g_DiffuseTexture.Sample(LinearWrap, membraneUV).rgb;
	float membraneDetail = dot(membrane, float3(0.299f, 0.587f, 0.114f));

	float impactRadius = lerp(0.08f, 0.72f, smoothstep(0.0f, 0.65f, ratio));
	float irregularity = (membraneDetail - 0.5f) * 0.16f +
		sin(angle * 9.0f + g_fAccumulationTime * 8.0f) * 0.025f;
	float distanceFromImpact = abs(radius - impactRadius - irregularity);
	float tornRim = 1.0f - smoothstep(0.025f, 0.105f, distanceFromImpact);

	float rippleA = 1.0f - smoothstep(0.015f, 0.07f,
		abs(radius - impactRadius - 0.15f));
	float rippleB = 1.0f - smoothstep(0.012f, 0.055f,
		abs(radius - impactRadius - 0.29f));
	float rippleFade = saturate(1.0f - ratio);

	float hole = 1.0f - smoothstep(impactRadius * 0.32f,
		impactRadius * 0.72f + 0.001f, radius);
	float membraneWave = smoothstep(0.28f, 0.8f, membraneDetail) *
		saturate(1.0f - radius) * (1.0f - hole);

	float fadeIn = smoothstep(0.0f, 0.06f, ratio);
	float fadeOut = 1.0f - smoothstep(0.58f, 1.0f, ratio);
	float mask = saturate(tornRim + rippleA * 0.55f + rippleB * 0.3f + membraneWave * 0.16f);
	mask *= fadeIn * fadeOut;

	clip(mask - 0.015f);
	float3 violet = float3(0.30f, 0.42f, 1.0f);
	float3 cyanWhite = float3(0.72f, 0.91f, 1.0f);
	float3 impactColor = lerp(violet, cyanWhite, saturate(tornRim + membraneDetail * 0.3f));
	impactColor *= tornRim * 4.2f + rippleA * 1.8f + rippleB + membraneWave * 0.55f;
	impactColor += In.vEmissive.rgb * In.vEmissive.a * mask * 0.35f;

	Out.vDiffuse = float4(impactColor * In.vColor.rgb, saturate(mask * In.vColor.a));
	return Out;
}

PS_OUT PSProtegoImpactLayered(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float2 centeredUV = In.vTexcoord * 2.0f - 1.0f;
	float radius = length(centeredUV);
	float angle = atan2(centeredUV.y, centeredUV.x);


	float pulseScale = lerp(1.18f, 0.82f, smoothstep(0.0f, 0.72f, ratio));
	float2 diffuseUV = centeredUV * pulseScale * 0.5f + 0.5f;
	diffuseUV += float2(
		g_fAccumulationTime * 0.035f,
		-g_fAccumulationTime * 0.022f);
	diffuseUV += float2(sin(angle * 7.0f), cos(angle * 9.0f)) * 0.012f;

	float2 anyUV = In.vTexcoord * 1.37f + float2(
		-g_fAccumulationTime * 0.11f,
		g_fAccumulationTime * 0.23f);
	anyUV += float2(
		sin(In.vTexcoord.y * 12.0f + g_fAccumulationTime * 1.7f),
		cos(In.vTexcoord.x * 10.0f - g_fAccumulationTime * 1.3f)) * 0.025f;

	float3 diffuseColor = g_DiffuseTexture.Sample(LinearWrap, diffuseUV).rgb;
	float3 anyColor = g_AnyTexture.Sample(LinearWrap, anyUV).rgb;
	float diffuseWhite = saturate(dot(diffuseColor, float3(0.299f, 0.587f, 0.114f)));
	float anyWhite = saturate(dot(anyColor, float3(0.299f, 0.587f, 0.114f)));
	float overlap = pow(saturate(diffuseWhite * anyWhite), 1.15f);

	float expansion = smoothstep(0.0f, 0.72f, ratio);
	float impactRadius = lerp(0.06f, 0.78f, expansion);
	float irregularity = (anyWhite - 0.5f) * 0.20f +
		sin(angle * 8.0f + anyWhite * 5.0f + g_fAccumulationTime * 6.0f) * 0.035f;
	float tornRim = 1.0f - smoothstep(0.025f, 0.12f,
		abs(radius - impactRadius - irregularity));
	float shockWave = 1.0f - smoothstep(0.018f, 0.075f,
		abs(radius - impactRadius - 0.19f - anyWhite * 0.06f));
	float outerWave = 1.0f - smoothstep(0.015f, 0.065f,
		abs(radius - impactRadius - 0.36f));

	float localDisc = 1.0f - smoothstep(0.72f, 1.05f, radius);
	float fadeIn = smoothstep(0.0f, 0.045f, ratio);
	float fadeOut = 1.0f - smoothstep(0.62f, 1.0f, ratio);
	float blastMask = smoothstep(0.055f, 0.56f, diffuseWhite) * localDisc;
	float membraneMask = saturate(diffuseWhite * 0.62f + anyWhite * 0.16f) * localDisc;
	float alpha = saturate(
		blastMask * 1.15f + membraneMask * 0.35f + tornRim * 0.72f +
		shockWave * 0.46f + outerWave * 0.24f + overlap * localDisc * 0.55f);
	alpha *= fadeIn * fadeOut * In.vColor.a;

	clip(alpha - 0.012f);
	float3 diffuseOriginal = diffuseColor;
	float3 violetTint = float3(0.40f, 0.06f, 0.78f);
	float3 magentaFlash = float3(0.96f, 0.12f, 1.0f);
	float3 overlapEmissive = lerp(violetTint, magentaFlash, anyWhite) *
		overlap * In.vEmissive.a * 5.4f;
	float3 rimEmissive = magentaFlash * tornRim * 2.8f +
		float3(0.48f, 0.28f, 1.0f) * (shockWave * 1.5f + outerWave * 0.75f);

	Out.vDiffuse = float4(
		diffuseOriginal * (0.85f + blastMask * 2.2f) + overlapEmissive + rimEmissive,
		alpha);
	return Out;
}

PS_OUT LumosWaver(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float2 centeredUV = In.vTexcoord * 2.f - 1.f;
	float radius = length(centeredUV);
	float angle = atan2(centeredUV.y, centeredUV.x);
	float time = g_fAccumulationTime;
	float2 noiseUV = In.vTexcoord * 1.7f + float2(time * 0.071f, -time * 0.053f);
	float2 noiseUV2 = In.vTexcoord * 3.1f + float2(-time * 0.043f, time * 0.067f);
	float noiseA = g_NoiseTexture.Sample(LinearWrap, noiseUV).r;
	float noiseB = g_NoiseTexture.Sample(LinearWrap, noiseUV2).g;
	float flowingNoise = noiseA * 0.65f + noiseB * 0.35f;
	float2 flowDirection = float2(noiseA, noiseB) * 2.f - 1.f;
	float edgeWeight = smoothstep(0.05f, 0.72f, radius);
	float2 distortedUV = In.vTexcoord + flowDirection * 0.018f * edgeWeight;
	distortedUV += float2(sin(time * 1.37f + centeredUV.y * 8.f),
		cos(time * 1.11f + centeredUV.x * 7.f)) * 0.0045f * edgeWeight;
	// Keep the flare large, then gently rotate and shear it so the long wisp rays
	// never look like a rigid billboard texture.
	float flareRotation = sin(time * 0.43f) * 0.11f + sin(time * 0.19f) * 0.055f;
	float flareCos = cos(flareRotation);
	float flareSin = sin(flareRotation);
	float2 flareCentered = distortedUV - 0.5f;
	flareCentered = float2(
		flareCentered.x * flareCos - flareCentered.y * flareSin,
		flareCentered.x * flareSin + flareCentered.y * flareCos);
	float wispBreath = 0.92f + sin(time * 1.31f + flowingNoise * 2.4f) * 0.035f;
	flareCentered.x *= wispBreath;
	flareCentered.y *= 1.84f - wispBreath;
	flareCentered += flowDirection * 0.012f * edgeWeight;
	float2 flareTextureUV = flareCentered * 0.96f + 0.5f;
	float insideFlareTexture =
		step(0.f, flareTextureUV.x) * step(flareTextureUV.x, 1.f) *
		step(0.f, flareTextureUV.y) * step(flareTextureUV.y, 1.f);
	float flareEdgeDistance = min(
		min(flareTextureUV.x, 1.f - flareTextureUV.x),
		min(flareTextureUV.y, 1.f - flareTextureUV.y));
	float flareEdgeFade = smoothstep(0.f, 0.09f, flareEdgeDistance) *
		insideFlareTexture;
	float4 flareTexel = g_DiffuseTexture.Sample(LinearClamp, flareTextureUV);
	float3 flareSample = flareTexel.rgb * flareTexel.a * flareEdgeFade;
	float flareLuminance = dot(flareSample, float3(0.299f, 0.587f, 0.114f));
	float textureBloom = pow(saturate(flareLuminance), 0.72f);
	float angleWarp = (flowingNoise - 0.5f) * 1.25f +
		sin(time * 1.9f + radius * 9.f) * 0.12f;
	float raysA = pow(saturate(0.5f + 0.5f *
		sin((angle + angleWarp) * 6.f + time * 0.73f)), 14.f);
	float raysB = pow(saturate(0.5f + 0.5f *
		sin((angle - angleWarp * 0.7f) * 11.f - time * 0.51f)), 22.f);
	float rayFade = pow(saturate(1.f - radius), 2.2f) *
		smoothstep(0.92f, 0.08f, radius);
	float rayMask = (raysA * 0.72f + raysB * 0.42f) * rayFade;
	float core = pow(saturate(1.f - radius), 12.f);
	float hotCore = pow(saturate(1.f - radius * 3.6f), 3.4f);
	float flicker = 0.88f + 0.12f * sin(time * 4.7f + flowingNoise * 6.283185f);
	float breathing = 0.91f + 0.09f * sin(time * 3.1f + flowingNoise * 4.f);
	float animatedFlare = textureBloom * breathing * lerp(0.86f, 1.14f, flowingNoise);
	float softOuterFade = 1.f - smoothstep(0.7f, 1.02f, radius);
	float outerSpread = animatedFlare * lerp(0.56f, 1.f,
		pow(saturate(1.f - radius), 2.2f)) * softOuterFade;
	float concentratedCore = pow(saturate(1.f - radius * 4.35f), 2.7f);
	float mask = saturate(max(outerSpread,
		concentratedCore * 1.28f + hotCore * 0.92f + core * 0.54f) +
		rayMask * 0.43f * flicker);
	float3 warmWhite = float3(1.f, 0.96f, 0.84f);
	float3 whiteCore = float3(1.f, 0.995f, 0.97f);
	float3 color = lerp(warmWhite, whiteCore, saturate(hotCore * 2.f));
	float4 emissive = lerp(In.vEmissive, In.vEndEmissive,
		saturate(In.life / max(In.maxLife, 0.0001f)));
	float centerFocus = pow(saturate(1.f - radius), 7.2f);
	float intensity = 0.94f + emissive.a * lerp(0.3f, 1.08f, centerFocus);
	float translucentEdge = lerp(0.115f, 0.94f, centerFocus);
	float alpha = saturate(max(concentratedCore, mask * translucentEdge) * In.vColor.a);

	Out.vDiffuse = float4(color * mask * intensity, alpha);
	clip(Out.vDiffuse.a - 0.004f);
	return Out;
}
PS_OUT TransformationLight(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float lifeRatio = saturate(1.f - In.life / max(In.maxLife, 0.0001f));
	float4 tex = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
	float mask = max(tex.r, max(tex.g, tex.b));
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);
	float3 diffuseColor = tex.rgb * In.vColor.rgb;
	float3 emissiveColor = lerpedEmissive.rgb * lerpedEmissive.a * mask;
	float3 finalColor = diffuseColor + emissiveColor;
	float finalAlpha = tex.a * mask * In.vColor.a;

	clip(finalAlpha - 0.002f);
	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}
PS_OUT PSDepulsoRing(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float lifeRatio = saturate(1.f - In.life / max(In.maxLife, 0.0001f));

	float2 uv = In.vTexcoord;
	float2 noiseUV1 = uv * 1.4f + float2(g_fAccumulationTime * 1.28f, -g_fAccumulationTime * 1.31f);
	float2 noiseUV2 = uv * 2.7f + float2(-g_fAccumulationTime * 1.43f, g_fAccumulationTime * 1.17f);

	float2 noise1 = g_NoiseTexture.Sample(LinearWrap, noiseUV1).rg * 2.f - 1.f;
	float2 noise2 = g_DistortionTexture.Sample(LinearWrap, noiseUV2).rg * 2.f - 1.f;
	float2 distortion = noise1 * 0.035f + noise2 * 0.018f;

	float2 warpedUV = uv + distortion;
	float3 ringTexture = g_DiffuseTexture.Sample(LinearClamp, warpedUV).rgb;
	float2 ringNormal = g_NormalTexture.Sample(LinearClamp, warpedUV).rg * 2.f - 1.f;
	float ringMask = max(ringTexture.r, max(ringTexture.g, ringTexture.b));

	float bodyMask = smoothstep(0.03f, 0.3f, ringMask);
	float coreMask = smoothstep(0.45f, 0.9f, ringMask);
	float glowMask = pow(saturate(ringMask), 0.65f);
	float normalDetail = saturate(length(ringNormal) * 1.4f);

	float colorNoise = g_NoiseTexture.Sample(LinearWrap, noiseUV2 + distortion).b;
	float breakNoise = g_NoiseTexture.Sample(LinearWrap, noiseUV1 * 1.7f).r;
	float breakMask = smoothstep(0.12f, 0.5f, breakNoise + ringMask);

	float3 deepBlue = float3(0.015f, 0.08f, 0.28f);
	float3 cyan = float3(0.02f, 0.65f, 1.f);
	float3 whiteBlue = float3(0.72f, 0.95f, 1.f);

	float3 ringColor = lerp(deepBlue, cyan, colorNoise);
	ringColor = lerp(ringColor, whiteBlue, saturate(coreMask * (0.65f + normalDetail * 0.35f)));

	float4 emissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);
	float3 baseColor = ringColor * bodyMask * In.vColor.rgb;
	float3 emissiveColor = ringColor * emissive.rgb * emissive.a * coreMask;

	float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
	screenUV = screenUV * float2(0.5f, -0.5f) + 0.5f;
	float2 refractionOffset = (ringNormal * 0.018f + distortion * 0.35f) * bodyMask;
	float3 refractedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + refractionOffset).rgb;
	float refractionMask = bodyMask * (1.f - coreMask) * 0.7f;
	float3 finalColor = lerp(baseColor, refractedBackground, refractionMask) + emissiveColor;

	float fadeIn = smoothstep(0.f, 0.08f, lifeRatio);
	float fadeOut = 1.f - smoothstep(0.72f, 1.f, lifeRatio);
	float finalAlpha = glowMask * breakMask * In.vColor.a * fadeIn * fadeOut;

	clip(finalAlpha - 0.005f);

	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}
