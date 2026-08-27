#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

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

StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);
Texture2D g_DiffuseTexture : register(t1);
Texture2D g_NormalTexture : register(t2);
Texture2D g_DistortionTexture : register(t3);
Texture2D g_NoiseTexture : register(t4);
Texture2D g_AnyTexture : register(t8);
Texture2D g_BackgroundTex : register(t7);

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

VS_OUT VSMain(uint vID : SV_VertexID, uint instID : SV_InstanceID)
{
	VS_OUT Out = (VS_OUT) 0;

	ParticleData p = g_RenderBuffer[instID];

	if (!p.alive)
	{
		Out.vColor = 0;
	}

	float2 baseUV = float2(vID % 2, 1 - (vID / 2));
	float2 finalUV = baseUV;

	if (g_iTotalFrames > 1 && g_iFlipbookColumns > 0 && g_iFlipbookRows > 0)
	{
		uint frame = min(p.frameIndex, g_iTotalFrames - 1);
		uint col = frame % g_iFlipbookColumns;
		uint row = frame / g_iFlipbookColumns;
		float2 uvSize = float2(1.0f / g_iFlipbookColumns, 1.0f / g_iFlipbookRows);
		float2 uvOffset = float2(col, row) * uvSize;

		finalUV = uvOffset + baseUV * uvSize;
	}

	Out.vTexcoord = finalUV;

	float3 camRight = g_matInvView[0].xyz;
	float3 camUp = g_matInvView[1].xyz;
	float3 camFwd = g_matInvView[2].xyz;

	float3 local = float3((baseUV - 0.5f) * p.size, 0);

	float4 vWorldPos;
	if ((p.iBehaviorType & BEHAVIOR_BILLBOARD) != 0)
	{
		float sinRot = sin(p.rotation.z);
		float cosRot = cos(p.rotation.z);
		float2 rotatedLocal = float2(local.x * cosRot - local.y * sinRot, local.x * sinRot + local.y * cosRot);
		float3 worldPos = p.position + camRight * rotatedLocal.x + camUp * rotatedLocal.y;

		vWorldPos = float4(worldPos, 1.f);
		Out.vNormal = -camFwd;
		Out.vTangent = camRight * cosRot + camUp * sinRot;
	}
	else
	{
		float3 rotatedLocal = RotateXYZ(local, p.rotation);

		float3 worldPos = p.position + rotatedLocal;
		vWorldPos = float4(worldPos, 1.0f);

		Out.vNormal = RotateXYZ(float3(0, 0, -1), p.rotation);
		Out.vTangent = RotateXYZ(float3(1, 0, 0), p.rotation);
	}

	float4 vViewPos = mul(vWorldPos, g_matView);
	Out.vPosition = mul(vViewPos, g_matProj);
	Out.vScreenPos = Out.vPosition;
	Out.vWorldPos = vWorldPos.xyz;
	Out.vColor = p.color;
	Out.vEmissive = p.emissive;
	Out.vEndEmissive = p.endEmissive;
	Out.iBehaviorType = p.iBehaviorType;
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
    
	float4 vTextureColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
	if (all(vTextureColor.a <= 0.03f))
		discard;
	if (all(vTextureColor.rgb <= 0.1f))
		discard;
	float ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
	{
		clip(vTextureColor.a - 0.02f);
		clip(In.vColor.a - 0.02f);

		float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
		screenUV.x = screenUV.x * 0.5f + 0.5f;
		screenUV.y = -screenUV.y * 0.5f + 0.5f;

		float4 vDistortionColor = g_DistortionTexture.Sample(LinearWrap, In.vTexcoord);
		float2 distortion = vDistortionColor.rg * 2.0f - 1.0f;

		float fEdgeMask = smoothstep(0.0f, 0.3f, vTextureColor.a) *
                          (1.0f - smoothstep(0.3f, 0.9f, vTextureColor.a));

		float distortionStrength = 0.01f * In.vColor.a * fEdgeMask;

		distortion *= distortionStrength;
		float4 distortedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion);
		float4 vFinalColor = vTextureColor * In.vColor;
		float3 finalRGB = lerp(distortedBackground.rgb, vFinalColor.rgb, vFinalColor.a);
		finalRGB += lerpedEmissive.rgb * lerpedEmissive.a;

		Out.vDiffuse = float4(finalRGB, 1.0f);
		return Out;
	}

	float4 vFinalColor = vTextureColor * In.vColor;
	clip(vFinalColor.a - 0.02f);



	float3 instEmissive = lerpedEmissive.rgb * lerpedEmissive.a;
	float3 FinalColor = vFinalColor.rgb + instEmissive;

	Out.vDiffuse = float4(FinalColor, vFinalColor.a);
	
	return Out;
}

PS_OUT RemoveBlack(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));

	float4 texColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);

	if (all(texColor.rgb < 0.1f))
		discard;
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);

	float3 finalRGB = texColor.rgb * In.vColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;
	
	Out.vDiffuse = float4(finalRGB, texColor.a * In.vColor.a);
	return Out;
}

PS_OUT PSFlameRing(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	
	float4 texColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
	float ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	
	float3 finalRGB = texColor.rgb * In.vColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;
	
	
	float2 centerUV = In.vTexcoord * 2.f - 1.f;
	float radius = length(centerUV);
	float angle = atan2(centerUV.y, centerUV.x) / 6.2831853f + 0.5f;

	float outerRadius = 0.82f;
	float ringWidth = 0.025f;

	float outerRing = 1.f - smoothstep(ringWidth, ringWidth + 0.015f, abs(radius - outerRadius));

	float2 noiseUV1 = float2(angle * 6.f - g_fDeltaTime * 0.35f, radius * 3.f - g_fDeltaTime * 0.8f);
	float2 noiseUV2 = float2(angle * 13.f + g_fDeltaTime * 0.2f, radius * 5.f + g_fDeltaTime * 0.45f);

	float noise1 = g_NoiseTexture.Sample(LinearWrap, noiseUV1).r;
	float noise2 = g_NoiseTexture.Sample(LinearWrap, noiseUV2).r;
	float noise = saturate(noise1 * 0.7f + noise2 * 0.3f);

	float flameMinDepth = 0.06f;
	float flameMaxDepth = 1.f;
	float flameDepth = lerp(flameMinDepth, flameMaxDepth, noise);

	float innerRadius = outerRadius - flameDepth;

	float flameMask = smoothstep(innerRadius, innerRadius + 0.04f, radius);
	flameMask *= 1.f - smoothstep(outerRadius, outerRadius + 0.02f, radius);

	float edgeProgress = saturate((radius - innerRadius) / max(flameDepth, 0.0001f));
	float flameDetail = pow(edgeProgress, 1.5f);
	flameMask *= flameDetail;

	float pulse = 0.85f + sin(g_fDeltaTime * 8.f + angle * 25.f) * 0.15f;
	flameMask *= pulse;

	float finalMask = saturate(max(outerRing, flameMask));

	clip(finalMask - 0.01f);


	//float3 flameColor = lerp(innerColor, outerColor, edgeProgress);
	//flameColor += outerRing * float3(1.f, 1.f, 1.f);
	
	Out.vDiffuse = float4(finalRGB * finalMask, finalMask * In.vColor.a);

	return Out;
}

PS_OUT PSDragonBreath(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float lifeRatio = saturate(1.f - In.life / max(In.maxLife, 0.0001f));
	float4 tex = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
	float mask = max(max(tex.r, tex.g), tex.b);

	clip(mask - 0.02f);

	float bodyMask = smoothstep(0.02f, 0.5f, mask);
	float emissiveMask = pow(saturate(mask), 1.5f);
	float4 emissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);

	float3 bodyColor = In.vColor.rgb;
	float3 emissiveColor = emissive.rgb * emissive.a * emissiveMask;
	float3 finalColor = bodyColor + emissiveColor;
	float finalAlpha = saturate(bodyMask * In.vColor.a);

	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}
PS_OUT PSRanrokSmoke(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float lifeRatio = saturate(1.f - In.life / max(In.maxLife, 0.0001f));
	float fadeIn = smoothstep(0.f, 0.08f, lifeRatio);
	float fadeOut = 1.f - smoothstep(0.3f, 1.f, lifeRatio);

	float2 noiseUV1 = In.vTexcoord * 1.4f + float2(g_fTime * 0.035f, -g_fTime * 0.08f);
	float2 noiseUV2 = In.vTexcoord * 2.7f + float2(-g_fTime * 0.06f, -g_fTime * 0.13f);

	float2 noise1 = g_DistortionTexture.Sample(LinearWrap, noiseUV1).rg * 2.f - 1.f;
	float2 noise2 = g_DistortionTexture.Sample(LinearWrap, noiseUV2).gb * 2.f - 1.f;
	float2 flowOffset = noise1 * 0.045f + noise2 * 0.018f;

	float2 smokeUV = In.vTexcoord;
	smokeUV.x += sin(In.vTexcoord.y * 7.f + g_fTime * 1.3f) * 0.025f;
	smokeUV += flowOffset * saturate(0.4f + lifeRatio);

	float4 textureColor = g_DiffuseTexture.Sample(LinearWrap, smokeUV);
	float textureLuminance = dot(textureColor.rgb, float3(0.299f, 0.587f, 0.114f));
	float smokeMask = max(textureColor.a, textureLuminance);

	float dissolveNoise = g_DistortionTexture.Sample(LinearWrap, noiseUV2 + flowOffset).b;
	float dissolveAmount = smoothstep(0.45f, 1.f, lifeRatio);
	float dissolveMask = smoothstep(dissolveAmount - 0.18f, dissolveAmount + 0.18f, dissolveNoise);

	float edgeMask = 1.f - smoothstep(0.f, 0.18f, abs(dissolveNoise - dissolveAmount));
	float finalAlpha = smokeMask * dissolveMask * fadeIn * fadeOut * In.vColor.a;

	clip(finalAlpha - 0.01f);

	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, lifeRatio);
	float3 smokeColor = textureColor.rgb * In.vColor.rgb;
	float3 emissiveColor = lerpedEmissive.rgb * lerpedEmissive.a * edgeMask * finalAlpha * 0.15f;

	if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
	{
		float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
		screenUV = screenUV * float2(0.5f, -0.5f) + 0.5f;

		float2 distortion = flowOffset * 0.35f * finalAlpha;
		float3 distortedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion).rgb;
		float3 finalColor = lerp(distortedBackground, smokeColor, finalAlpha) + emissiveColor;

		Out.vDiffuse = float4(finalColor, finalAlpha);
		return Out;
	}

	Out.vDiffuse = float4(smokeColor + emissiveColor, finalAlpha);
	return Out;
}
PS_OUT PSRanrokSmokeWispy(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ageRatio = saturate(1.f - In.life / max(In.maxLife, 0.0001f));
	float fadeIn = smoothstep(0.f, 0.18f, ageRatio);
	float fadeOut = 1.f - smoothstep(0.35f, 0.95f, ageRatio);
	float lifeFade = fadeIn * fadeOut;

	float2 uv = In.vTexcoord;
	float2 centeredUV = uv * 2.f - 1.f;

	float smokeBallMask = g_AnyTexture.Sample(LinearClamp, uv).r;
	float circleMask = 1.f - smoothstep(0.65f, 1.f, length(centeredUV));

	float2 distortionUV1 = uv + ageRatio * float2(0.4f, 0.2f);
	float2 distortion1 = g_NoiseTexture.Sample(LinearWrap, distortionUV1).rg * 2.f - 1.f;
	float distortionEdge = smoothstep(0.15f, 0.85f, smokeBallMask);
	float distortionStrength1 = lerp(0.2f, -0.157143f, distortionEdge);
	float2 wispUV1 = uv * float2(0.5f, 0.5f) + ageRatio * float2(0.03f, 0.01f);
	wispUV1 += distortion1 * distortionStrength1;

	float3 wispTexture1 = g_DiffuseTexture.Sample(LinearWrap, wispUV1).rgb;
	float wispMask1 = dot(wispTexture1, float3(0.299f, 0.587f, 0.114f));
	wispMask1 = pow(saturate(wispMask1), 0.55f);

	float smokeShapeMask = pow(saturate(smokeBallMask), 0.45f);
	float wispDensityMask = smoothstep(0.02f, 0.35f, wispMask1);
	float bodyMask = smokeShapeMask * lerp(0.65f, 1.f, wispDensityMask);

	float2 distortionUV2 = uv * 0.9f + ageRatio * float2(0.1f, 0.05f);
	float2 distortion2 = g_NoiseTexture.Sample(LinearWrap, distortionUV2).rg * 2.f - 1.f;
	float2 wispUV2 = uv * float2(0.45f, 0.45f) + ageRatio * float2(-0.02f, -0.01f);
	wispUV2 += distortion2 * 0.06f;

	float3 wispTexture2 = g_DiffuseTexture.Sample(LinearWrap, wispUV2).rgb;
	float wispMask2 = dot(wispTexture2, float3(0.299f, 0.587f, 0.114f));
	float thinWispMask = smoothstep(0.45f, 0.82f, wispMask2);
	thinWispMask *= circleMask;

	float4 emissive = lerp(In.vEmissive, In.vEndEmissive, ageRatio);
	float3 blackSmoke = lerp(float3(0.018f, 0.004f, 0.022f), In.vColor.rgb, 0.65f);
	float3 smokeBody = blackSmoke;
	float3 glowingWisp = emissive.rgb * emissive.a * thinWispMask * 0.15f;
	float3 finalColor = smokeBody + glowingWisp;

	float shapeMask = thinWispMask;
	clip(shapeMask - 0.04f);

	float finalAlpha = In.vColor.a * lifeFade;
	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}
PS_OUT PSDashWisp(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ageRatio = saturate(
		In.life / max(In.maxLife, 0.0001f));

	// 처음에는 서서히 나타나고 마지막에는 사라진다.
	float fadeIn = smoothstep(0.f, 0.08f, ageRatio);
	float fadeOut = 1.f - smoothstep(0.58f, 1.f, ageRatio);
	float lifeFade = fadeIn * fadeOut;

	float2 uv = In.vTexcoord;

	// 서로 다른 방향으로 움직이는 노이즈 두 장
	float2 noiseUV1 =
		uv * 1.4f +
		float2(
			g_fTime * 1.47f,
			g_fTime * 1.71f);

	float2 noiseUV2 =
		uv * 2.6f +
		float2(
			-g_fTime * 1.49f,
			g_fTime * 1.35f);

	float2 distortion1 =
		g_DistortionTexture.Sample(
			LinearWrap, noiseUV1).rg * 2.f - 1.f;

	float2 distortion2 =
		g_NoiseTexture.Sample(
			LinearWrap, noiseUV2).rg * 2.f - 1.f;

	// X 왜곡을 크게 해서 좌우로 흐느적거리게 한다.
	float2 distortion =
		distortion1 * float2(0.345f, 0.318f) +
		distortion2 * float2(0.322f, 0.312f);

	// 수명이 지날수록 조금 더 흐트러진다.
	float2 distortedUV =
		uv + distortion * lerp(0.65f, 1.35f, ageRatio);

	float3 diffuseTexture =
		g_DiffuseTexture.Sample(
			LinearClamp, distortedUV).rgb;

	float luminance = dot(
		diffuseTexture,
		float3(0.299f, 0.587f, 0.114f));

	// 어두운 배경은 제거하고 희미한 수증기는 유지한다.
	float bodyMask =
		smoothstep(0.008f, 0.22f, luminance);

	bodyMask = pow(saturate(bodyMask), 0.72f);

	float breakupNoise =
		g_NoiseTexture.Sample(
			LinearWrap,
			uv * 1.8f +
			float2(
				-g_fTime * 0.035f,
				g_fTime * 0.06f)).b;

	float breakupMask =
		lerp(0.72f, 1.f, breakupNoise);

	float density =
		bodyMask *
		breakupMask *
		lifeFade;

	// 텍스처 사각형 테두리가 보이지 않도록 처리
	float edgeFade =
		smoothstep(0.f, 0.04f, uv.x) *
		(1.f - smoothstep(0.96f, 1.f, uv.x)) *
		smoothstep(0.f, 0.04f, uv.y) *
		(1.f - smoothstep(0.96f, 1.f, uv.y));

	density *= edgeFade;

	float brightMask =
		smoothstep(0.35f, 0.85f, bodyMask);

	float4 emissive =
		lerp(
			In.vEmissive,
			In.vEndEmissive,
			ageRatio);

	float3 vaporColor =
		In.vColor.rgb *
		lerp(0.45f, 1.f, bodyMask);

	// 밝은 결에만 약한 이미시브
	float3 emissiveColor =
		emissive.rgb *
		emissive.a *
		brightMask *
		0.18f;

	float3 finalColor =
		vaporColor +
		emissiveColor;
	float finalAlpha =
	saturate(density * In.vColor.a * 0.5f);

	clip(finalAlpha - 0.002f);

	Out.vDiffuse =
		float4(finalColor, finalAlpha);

	return Out;
}
PS_OUT PSBreathFireWisp(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ageRatio = saturate(
		1.f - In.life / max(In.maxLife, 0.0001f));

	float fadeIn =
		smoothstep(0.f, 0.06f, ageRatio);

	float fadeOut =
		1.f - smoothstep(0.62f, 1.f, ageRatio);

	float lifeFade = fadeIn * fadeOut;

	float2 uv = In.vTexcoord;

	float2 distortionUV1 =
		uv * float2(1.3f, 1.8f) +
		float2(
			g_fTime * 0.09f,
			-g_fTime * 0.32f);

	float2 distortionUV2 =
		uv * float2(2.7f, 3.4f) +
		float2(
			-g_fTime * 0.15f,
			g_fTime * 0.21f);

	float2 distortion1 =
		g_DistortionTexture.Sample(
			LinearWrap, distortionUV1).rg * 2.f - 1.f;

	float2 distortion2 =
		g_NoiseTexture.Sample(
			LinearWrap, distortionUV2).rg * 2.f - 1.f;

	float2 distortion =
		distortion1 * float2(0.045f, 0.012f) +
		distortion2 * float2(0.018f, 0.008f);

	float2 distortedUV =
		uv + distortion * lerp(0.6f, 1.3f, ageRatio);

	float4 fireTexture =
		g_DiffuseTexture.Sample(
			LinearClamp, distortedUV);

	// 이 텍스처는 정상적인 알파 채널을 가지고 있다.
	float shapeMask =
		pow(saturate(fireTexture.a), 0.72f);

	float fireLuminance =
		dot(
			fireTexture.rgb,
			float3(0.299f, 0.587f, 0.114f));

	float detailMask =
		smoothstep(0.04f, 0.5f, fireLuminance);

	float breakupNoise =
		g_NoiseTexture.Sample(
			LinearWrap,
			uv * float2(1.6f, 2.8f) +
			float2(
				g_fTime * 0.04f,
				-g_fTime * 0.24f)).r;

	float breakupMask =
		lerp(0.55f, 1.f, breakupNoise);

	float edgeFade =
		smoothstep(0.f, 0.08f, uv.x) *
		(1.f - smoothstep(0.92f, 1.f, uv.x)) *
		smoothstep(0.f, 0.06f, uv.y) *
		(1.f - smoothstep(0.94f, 1.f, uv.y));

	float finalMask =
		shapeMask *
		breakupMask *
		edgeFade *
		lifeFade;

	float coreMask =
		smoothstep(0.35f, 0.82f, detailMask * shapeMask);

	float4 emissive =
		lerp(
			In.vEmissive,
			In.vEndEmissive,
			ageRatio);

	// 텍스처의 원래 주황색과 인스턴스의 붉은색을 혼합한다.
	float3 fireTint =
		lerp(
			In.vColor.rgb,
			fireTexture.rgb,
			0.42f);

	float3 bodyColor =
		fireTint *
		lerp(0.25f, 1.f, detailMask) *
		finalMask;

	float3 emissiveColor =
		emissive.rgb *
		emissive.a *
		coreMask *
		1.2f;

	float3 finalColor =
		bodyColor + emissiveColor;

	float finalAlpha =
		saturate(
			finalMask *
			In.vColor.a *
			0.85f);

	clip(finalAlpha - 0.002f);

	Out.vDiffuse =
		float4(finalColor, finalAlpha);

	return Out;
}

PS_OUT PSBreathWispySmoke(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ageRatio = saturate(
		1.f - In.life / max(In.maxLife, 0.0001f));

	float fadeIn =
		smoothstep(0.f, 0.12f, ageRatio);

	float fadeOut =
		1.f - smoothstep(0.55f, 1.f, ageRatio);

	float lifeFade = fadeIn * fadeOut;

	// 현재 플립북 프레임 내부의 로컬 UV를 복원한다.
	float2 atlasCount = float2(
		max(g_iFlipbookColumns, 1u),
		max(g_iFlipbookRows, 1u));

	float2 atlasPosition =
		In.vTexcoord * atlasCount;

	float2 cellIndex =
		floor(atlasPosition - 0.0001f);

	cellIndex = clamp(
		cellIndex,
		float2(0.f, 0.f),
		atlasCount - 1.f);

	float2 localUV =
		saturate(atlasPosition - cellIndex);

	float2 noiseUV1 =
		localUV * 1.4f +
		float2(
			g_fTime * 0.035f,
			-g_fTime * 0.07f);

	float2 noiseUV2 =
		localUV * 2.6f +
		float2(
			-g_fTime * 0.055f,
			g_fTime * 0.045f);

	float2 distortion1 =
		g_DistortionTexture.Sample(
			LinearWrap, noiseUV1).rg * 2.f - 1.f;

	float2 distortion2 =
		g_NoiseTexture.Sample(
			LinearWrap, noiseUV2).rg * 2.f - 1.f;

	float2 distortion =
		distortion1 * 0.035f +
		distortion2 * 0.016f;

	float2 distortedLocalUV =
		clamp(
			localUV +
			distortion * lerp(0.5f, 1.25f, ageRatio),
			0.015f,
			0.985f);

	// 왜곡 후에도 현재 프레임 셀 내부만 샘플링한다.
	float2 smokeUV =
		(cellIndex + distortedLocalUV) /
		atlasCount;

	float3 smokeTexture =
		g_DiffuseTexture.Sample(
			LinearClamp, smokeUV).rgb;

	// 이 텍스처의 알파는 전체가 1이므로 RGB 밝기로 마스크를 만든다.
	float smokeLuminance =
		dot(
			smokeTexture,
			float3(0.299f, 0.587f, 0.114f));

	float smokeMask =
		smoothstep(0.025f, 0.38f, smokeLuminance);

	smokeMask =
		pow(saturate(smokeMask), 0.78f);

	float breakupNoise =
		g_NoiseTexture.Sample(
			LinearWrap,
			localUV * 1.7f +
			float2(
				-g_fTime * 0.025f,
				g_fTime * 0.06f)).r;

	float breakupMask =
		lerp(0.62f, 1.f, breakupNoise);

	float edgeFade =
		smoothstep(0.f, 0.07f, localUV.x) *
		(1.f - smoothstep(0.93f, 1.f, localUV.x)) *
		smoothstep(0.f, 0.07f, localUV.y) *
		(1.f - smoothstep(0.93f, 1.f, localUV.y));

	float finalMask =
		smokeMask *
		breakupMask *
		edgeFade *
		lifeFade;

	float4 emissive =
		lerp(
			In.vEmissive,
			In.vEndEmissive,
			ageRatio);

	float3 smokeColor =
		In.vColor.rgb *
		lerp(0.45f, 1.f, smokeMask);

	// 연기는 거의 발광시키지 않는다.
	float3 emissiveColor =
		emissive.rgb *
		emissive.a *
		smokeMask *
		0.08f;

	float3 finalColor =
		smokeColor + emissiveColor;

	float finalAlpha =
		saturate(
			finalMask *
			In.vColor.a *
			0.7f);

	clip(finalAlpha - 0.001f);

	Out.vDiffuse =
		float4(finalColor, finalAlpha);

	return Out;
}
PS_OUT PSFire(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;


	float ageRatio = saturate(
		In.life / max(In.maxLife, 0.0001f));
	// VS에서 Flipbook 프레임 UV가 적용된 상태
	float2 uv = In.vTexcoord;

	float4 fireTexture =
		g_DiffuseTexture.Sample(LinearClamp, uv);

	float fireMask = dot(
		fireTexture.rgb,
		float3(0.299f, 0.587f, 0.114f));

	// 8×8 Flipbook 셀 내부 좌표
	float2 localUV = frac(uv * float2(8.f, 8.f));

	float2 noiseUV =
		localUV * float2(1.3f, 2.1f) +
		float2(
			-g_fTime * 0.12f,
			-g_fTime * 0.65f);

	float noise =
		g_NoiseTexture.Sample(LinearWrap, noiseUV).r;

	// 불꽃 밝기가 고정적으로 반복되지 않게 변화
	fireMask *= lerp(0.72f, 1.15f, noise);
	fireMask = saturate(fireMask);

	float bodyMask = smoothstep(0.03f, 0.28f, fireMask);
	float hotMask = smoothstep(0.38f, 0.88f, fireMask);
	float coreMask = smoothstep(0.72f, 0.97f, fireMask);

	// 바깥쪽 적색 → 주황색 → 중심부 황백색
	float3 outerColor = float3(0.95f, 0.035f, 0.002f);
	float3 middleColor = float3(1.f, 0.28f, 0.01f);
	float3 coreColor = float3(1.f, 0.88f, 0.32f);

	float3 fireColor = lerp(outerColor, middleColor, hotMask);
	fireColor = lerp(fireColor, coreColor, coreMask);

	// 인스턴스 컬러로 전체 색조 조절
	fireColor *= max(In.vColor.rgb, 0.05f);

	float4 emissive =
		lerp(In.vEmissive, In.vEndEmissive, ageRatio);

	float3 baseColor =
		fireColor * bodyMask;

	float3 emissiveColor =
		fireColor *
		emissive.rgb *
		emissive.a *
		hotMask;

	float fadeIn = smoothstep(0.f, 0.08f, ageRatio);
	float fadeOut =
		1.f - smoothstep(0.72f, 1.f, ageRatio);

	float finalAlpha =
		bodyMask *
		In.vColor.a *
		fadeIn *
		fadeOut;

	clip(finalAlpha - 0.01f);

	Out.vDiffuse = float4(
		baseColor + emissiveColor,
		finalAlpha);

	return Out;
}
